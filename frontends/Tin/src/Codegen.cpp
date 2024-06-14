//===- Codegen.cpp - Codegen for the Tin language -------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "tin/Codegen.h"

#include "circt/Dialect/Comb/CombOps.h"
#include "circt/Dialect/HW/HWOps.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/Diagnostics.h"
#include "mlir/IR/Verifier.h"
#include "llvm/ADT/ScopedHashTable.h"
#include "llvm/ADT/TypeSwitch.h"

using namespace circt;
using namespace tin;

namespace {
struct Codegen {
  ModuleOp module;
  MLIRContext *context;
  OpBuilder builder;
  SymbolTable symbolTable;

  using NamedValues = llvm::ScopedHashTable<ast::Binding, Value>;
  using NamedValueScope = NamedValues::ScopeTy;
  NamedValues namedValues;

  Codegen(ModuleOp module)
      : module(module), context(module.getContext()), builder(context),
        symbolTable(module) {
    builder.setInsertionPointToEnd(module.getBody());
  }

  LogicalResult visit(AST &ast) {
    for (auto *root : ast.roots)
      if (failed(visit(*root)))
        return failure();
    return success();
  }

  LogicalResult visit(ast::Root &root) {
    for (auto *item : root.items)
      if (failed(visit(*item)))
        return failure();
    return success();
  }

  //===--------------------------------------------------------------------===//
  // Items
  //===--------------------------------------------------------------------===//

  LogicalResult visit(ast::Item &item) {
    return TypeSwitch<ast::Item *, LogicalResult>(&item)
#define AST_ITEM(NAME)                                                         \
  .Case<ast::NAME##Item>([&](auto *item) { return visitItem(*item); })
#include "tin/AST.def"
        ;
  }

  LogicalResult visitItem(ast::ModItem &item) {
    NamedValueScope scope(namedValues);

    SmallVector<hw::PortInfo> ports;
    for (auto *astPort : item.ports) {
      hw::PortInfo irPort;
      irPort.loc = astPort->loc;
      irPort.dir =
          astPort->isOutput ? hw::PortInfo::Output : hw::PortInfo::Input;
      irPort.name = astPort->name;
      irPort.type = visit(*astPort->type);
      if (!irPort.type)
        return failure();
      ports.push_back(irPort);
    }

    auto mod = builder.create<hw::HWModuleOp>(item.loc, item.name, ports);
    symbolTable.insert(mod);

    OpBuilder::InsertionGuard g(builder);
    builder.setInsertionPointToStart(mod.getBodyBlock());

    // Add the ports to the map of named values such that identifier expressions
    // can find them.
    unsigned argIdx = 0;
    for (auto *port : item.ports) {
      if (port->isOutput)
        continue;
      namedValues.insert(port, mod.getBody().getArgument(argIdx));
      ++argIdx;
    }

    // Create placeholder values and wires for output ports and add them to the
    // map of named values.
    SmallVector<Operation *> placeholders;
    SmallVector<Value> outputOperands;
    for (auto *port : item.ports) {
      if (!port->isOutput)
        continue;
      auto placeholder = builder.create<mlir::UnrealizedConversionCastOp>(
          port->loc, TypeRange{visit(*port->type)}, ValueRange{});
      placeholders.push_back(placeholder);
      outputOperands.push_back(placeholder.getResult(0));
      namedValues.insert(port, placeholder.getResult(0));
    }
    cast<hw::OutputOp>(mod.getBodyBlock()->getTerminator())
        .getOutputsMutable()
        .assign(outputOperands);

    for (auto *stmt : item.stmts) {
      if (auto *letStmt = dyn_cast<ast::LetStmt>(stmt)) {
        auto type = visit(*letStmt->type);
        if (!type)
          return failure();
        auto placeholder = builder.create<mlir::UnrealizedConversionCastOp>(
            stmt->loc, TypeRange{type}, ValueRange{});
        placeholders.push_back(placeholder);
        namedValues.insert(letStmt, placeholder.getResult(0));
      }
    }

    for (auto *stmt : item.stmts)
      if (failed(visit(*stmt)))
        return failure();

    // Remove the temporary wires created for the output ports.
    bool anyErrors = false;
    for (auto *port : item.ports) {
      if (!port->isOutput)
        continue;
      auto *placeholderOp = namedValues.lookup(port).getDefiningOp();
      if (placeholderOp->getNumOperands() != 1) {
        auto d = mlir::emitError(port->loc) << "port `" << port->name.getValue()
                                            << "` has not been assigned";
        d.attachNote() << "hint: add a `out " << port->name.getValue()
                       << " = <value>;` statement";
        anyErrors = true;
      }
    }
    if (anyErrors)
      return failure();

    // Remove placeholders created for ports and declarations.
    for (auto *placeholder : placeholders) {
      assert(placeholder->getNumOperands() == 1);
      placeholder->getResult(0).replaceAllUsesWith(placeholder->getOperand(0));
      placeholder->erase();
    }

    return success();
  }

  //===--------------------------------------------------------------------===//
  // Statements
  //===--------------------------------------------------------------------===//

  LogicalResult visit(ast::Stmt &stmt) {
    return TypeSwitch<ast::Stmt *, LogicalResult>(&stmt)
#define AST_STMT(NAME)                                                         \
  .Case<ast::NAME##Stmt>([&](auto *stmt) { return visitStmt(*stmt); })
#include "tin/AST.def"
        ;
  }

  LogicalResult visitStmt(ast::EmptyStmt &stmt) { return success(); }

  LogicalResult visitStmt(ast::ExprStmt &stmt) {
    auto value = visit(*stmt.expr);
    if (!value)
      return failure();
    return success();
  }

  LogicalResult visitStmt(ast::OutStmt &stmt) {
    auto value = visit(*stmt.value);
    if (!value)
      return failure();
    assert(stmt.binding);
    auto *placeholderOp = namedValues.lookup(stmt.binding).getDefiningOp();
    placeholderOp->setOperands(value);
    return success();
  }

  LogicalResult visitStmt(ast::LetStmt &stmt) {
    auto value = visit(*stmt.value);
    if (!value)
      return failure();
    auto *placeholderOp = namedValues.lookup(&stmt).getDefiningOp();
    placeholderOp->setOperands(value);
    return success();
  }

  //===--------------------------------------------------------------------===//
  // Expressions
  //===--------------------------------------------------------------------===//

  Value visit(ast::Expr &expr) {
    return TypeSwitch<ast::Expr *, Value>(&expr)
#define AST_EXPR(NAME)                                                         \
  .Case<ast::NAME##Expr>([&](auto *expr) { return visitExpr(*expr); })
#include "tin/AST.def"
        ;
  }

  Value visitExpr(ast::NumLitExpr &expr) {
    return builder.create<hw::ConstantOp>(expr.loc, expr.value);
  }

  Value visitExpr(ast::IdentExpr &expr) {
    assert(expr.binding);
    auto value = namedValues.lookup(expr.binding);
    assert(value);
    return value;
  }

  Value visitExpr(ast::ParenExpr &expr) { return visit(*expr.expr); }

  Value visitExpr(ast::UnaryExpr &expr) {
    auto arg = visit(*expr.arg);
    if (!arg)
      return {};

    switch (expr.op) {
    case ast::UnaryOp::Neg: {
      auto zero = builder.create<hw::ConstantOp>(expr.loc, arg.getType(), 0);
      return builder.create<comb::SubOp>(expr.loc, zero, arg);
    }
    case ast::UnaryOp::Not: {
      auto ones = builder.create<hw::ConstantOp>(expr.loc, arg.getType(), -1);
      return builder.create<comb::XorOp>(expr.loc, ones, arg);
    }
    }

    mlir::emitError(expr.loc) << "unary expression codegen not implemented";
    return {};
  }

  Value visitExpr(ast::BinaryExpr &expr) {
    auto lhs = visit(*expr.lhs);
    if (!lhs)
      return {};
    auto rhs = visit(*expr.rhs);
    if (!rhs)
      return {};

    switch (expr.op) {
    case ast::BinaryOp::And:
      return builder.create<comb::AndOp>(expr.loc, lhs, rhs);
    case ast::BinaryOp::Or:
      return builder.create<comb::OrOp>(expr.loc, lhs, rhs);
    case ast::BinaryOp::Xor:
      return builder.create<comb::XorOp>(expr.loc, lhs, rhs);
    case ast::BinaryOp::Add:
      return builder.create<comb::AddOp>(expr.loc, lhs, rhs);
    case ast::BinaryOp::Sub:
      return builder.create<comb::SubOp>(expr.loc, lhs, rhs);
    case ast::BinaryOp::Mul:
      return builder.create<comb::MulOp>(expr.loc, lhs, rhs);
    case ast::BinaryOp::Div:
      return builder.create<comb::DivUOp>(expr.loc, lhs, rhs);
    case ast::BinaryOp::Mod:
      return builder.create<comb::ModUOp>(expr.loc, lhs, rhs);
    case ast::BinaryOp::Shl:
      return builder.create<comb::ShlOp>(expr.loc, lhs, rhs);
    case ast::BinaryOp::Shr:
      return builder.create<comb::ShrUOp>(expr.loc, lhs, rhs);
    case ast::BinaryOp::Eq:
      return builder.create<comb::ICmpOp>(expr.loc, comb::ICmpPredicate::eq,
                                          lhs, rhs);
    case ast::BinaryOp::Neq:
      return builder.create<comb::ICmpOp>(expr.loc, comb::ICmpPredicate::ne,
                                          lhs, rhs);
    case ast::BinaryOp::Lt:
      return builder.create<comb::ICmpOp>(expr.loc, comb::ICmpPredicate::ult,
                                          lhs, rhs);
    case ast::BinaryOp::Gt:
      return builder.create<comb::ICmpOp>(expr.loc, comb::ICmpPredicate::ugt,
                                          lhs, rhs);
    case ast::BinaryOp::Leq:
      return builder.create<comb::ICmpOp>(expr.loc, comb::ICmpPredicate::ule,
                                          lhs, rhs);
    case ast::BinaryOp::Geq:
      return builder.create<comb::ICmpOp>(expr.loc, comb::ICmpPredicate::uge,
                                          lhs, rhs);
    }

    mlir::emitError(expr.loc) << "binary expression codegen not implemented";
    return {};
  }

  //===--------------------------------------------------------------------===//
  // Types
  //===--------------------------------------------------------------------===//

  Type visit(ast::Type &type) {
    if (auto *intType = dyn_cast<ast::IntType>(&type))
      return builder.getIntegerType(intType->width);

    mlir::emitError(type.loc) << "type codegen not implemented";
    return {};
  }
};
} // namespace

OwningOpRef<ModuleOp> tin::convertToMLIR(MLIRContext *context, AST &ast) {
  context->loadDialect<hw::HWDialect, comb::CombDialect>();
  auto module = ModuleOp::create(UnknownLoc::get(context));
  Codegen codegen(module);
  if (failed(codegen.visit(ast)))
    return {};
  if (failed(verify(module))) {
    module.emitError("module verification error");
    return {};
  }
  return module;
}
