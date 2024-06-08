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
#include "llvm/ADT/TypeSwitch.h"

using namespace circt;
using namespace tin;

namespace {
struct Codegen {
  ModuleOp module;
  MLIRContext *context;
  OpBuilder builder;
  SymbolTable symbolTable;

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
        .Default([&](auto *) {
          return mlir::emitError(item.loc) << "item codegen not implemented";
        });
  }

  LogicalResult visitItem(ast::ModItem &item) {
    auto mod = builder.create<hw::HWModuleOp>(item.loc, item.name,
                                              ArrayRef<hw::PortInfo>{});
    symbolTable.insert(mod);

    OpBuilder::InsertionGuard g(builder);
    builder.setInsertionPointToStart(mod.getBodyBlock());

    for (auto *stmt : item.stmts)
      if (failed(visit(*stmt)))
        return failure();

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
        .Default([&](auto *) {
          return mlir::emitError(stmt.loc)
                 << "statement codegen not implemented";
        });
  }

  LogicalResult visitStmt(ast::EmptyStmt &stmt) { return success(); }

  LogicalResult visitStmt(ast::ExprStmt &stmt) {
    auto value = visit(*stmt.expr);
    if (!value)
      return failure();
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
        .Default([&](auto *) {
          mlir::emitError(expr.loc) << "expression codegen not implemented";
          return Value{};
        });
  }

  Value visitExpr(ast::NumLitExpr &expr) {
    return builder.create<hw::ConstantOp>(expr.loc, expr.value);
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
