//===- Codegen.cpp - Codegen for the Tin language -------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "tin/Codegen.h"

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
        .Case<ast::ModItem>([&](auto *item) { return visit(*item); })
        .Default([&](auto *) {
          return mlir::emitError(item.loc) << "item codegen not implemented";
        });
  }

  LogicalResult visit(ast::ModItem &item) {
    auto mod = builder.create<hw::HWModuleOp>(item.loc, item.name,
                                              ArrayRef<hw::PortInfo>{});
    symbolTable.insert(mod);
    return success();
  }
};
} // namespace

OwningOpRef<ModuleOp> tin::convertToMLIR(MLIRContext *context, AST &ast) {
  context->loadDialect<hw::HWDialect>();
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
