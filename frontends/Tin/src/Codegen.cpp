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

    using ast::UnaryOp;
    switch (expr.op) {
    case UnaryOp::Neg: {
      auto zero = builder.create<hw::ConstantOp>(expr.loc, arg.getType(), 0);
      return builder.create<comb::SubOp>(expr.loc, zero, arg);
    }
    case UnaryOp::Not: {
      auto ones = builder.create<hw::ConstantOp>(expr.loc, arg.getType(), -1);
      return builder.create<comb::XorOp>(expr.loc, ones, arg);
    }
    }

    mlir::emitError(expr.loc) << "unary expression codegen not implemented";
    return {};
  }

  Value visitExpr(ast::BinaryExpr &expr) {
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
  if (failed(module.verify())) {
    module.emitError("module verification error");
    return {};
  }
  return module;
}
