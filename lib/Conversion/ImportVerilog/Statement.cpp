//===- Statement.cpp - Slang statement conversion--------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "ImportVerilogInternals.h"
#include "slang/ast/ASTVisitor.h"
#include "slang/ast/Symbol.h"
#include "slang/ast/symbols/CompilationUnitSymbols.h"
#include "slang/ast/symbols/InstanceSymbols.h"
#include "slang/ast/symbols/VariableSymbols.h"
#include "slang/ast/types/AllTypes.h"
#include "slang/ast/types/Type.h"
#include "slang/syntax/SyntaxVisitor.h"

using namespace circt;
using namespace ImportVerilog;

LogicalResult Context::visitConditionalStmt(
    const slang::ast::ConditionalStatement *conditionalStmt) {
  auto loc = convertLocation(conditionalStmt->sourceRange.start());
  Value cond = visitExpression(conditionalStmt->conditions.begin()->expr);
  if (!cond)
    return failure();

  auto ifOp = rootBuilder.create<moore::IfOp>(
      loc, cond, [&]() { convertStatement(&conditionalStmt->ifTrue); },
      [&]() {});
  if (ifOp.hasElse()) {
    rootBuilder.setInsertionPointToEnd(ifOp.getElseBlock());
    if (conditionalStmt->ifFalse)
      convertStatement(conditionalStmt->ifFalse);
    else
      ifOp.getElseBlock()->erase();
  }
  return success();
}

// It can handle the statements like case, conditional(if), for loop, and etc.
LogicalResult
Context::convertStatement(const slang::ast::Statement *statement) {
  auto loc = convertLocation(statement->sourceRange.start());
  switch (statement->kind) {
  case slang::ast::StatementKind::List:
    for (auto *stmt : statement->as<slang::ast::StatementList>().list) {
      convertStatement(stmt);
    }
    break;
  case slang::ast::StatementKind::Block:
    convertStatement(&statement->as<slang::ast::BlockStatement>().body);
    break;
  case slang::ast::StatementKind::ExpressionStatement:
    visitExpression(&statement->as<slang::ast::ExpressionStatement>().expr);
    break;
  case slang::ast::StatementKind::VariableDeclaration:
    return mlir::emitError(loc, "unsupported statement: variable declaration");
  case slang::ast::StatementKind::Return:
    return mlir::emitError(loc, "unsupported statement: return");
  case slang::ast::StatementKind::Break:
    return mlir::emitError(loc, "unsupported statement: break");
  case slang::ast::StatementKind::Continue:
    return mlir::emitError(loc, "unsupported statement: continue");
  case slang::ast::StatementKind::Case:
    return mlir::emitError(loc, "unsupported statement: case");
  case slang::ast::StatementKind::PatternCase:
    return mlir::emitError(loc, "unsupported statement: pattern case");
  case slang::ast::StatementKind::ForLoop:
    return mlir::emitError(loc, "unsupported statement: for loop");
  case slang::ast::StatementKind::RepeatLoop:
    return mlir::emitError(loc, "unsupported statement: repeat loop");
  case slang::ast::StatementKind::ForeachLoop:
    return mlir::emitError(loc, "unsupported statement: foreach loop");
  case slang::ast::StatementKind::WhileLoop:
    return mlir::emitError(loc, "unsupported statement: while loop");
  case slang::ast::StatementKind::DoWhileLoop:
    return mlir::emitError(loc, "unsupported statement: do while loop");
  case slang::ast::StatementKind::ForeverLoop:
    return mlir::emitError(loc, "unsupported statement: forever loop");
  case slang::ast::StatementKind::Timed:
    return mlir::emitError(loc, "unsupported statement: timed");
  case slang::ast::StatementKind::ImmediateAssertion:
    return mlir::emitError(loc, "unsupported statement: immediate assertion");
  case slang::ast::StatementKind::ConcurrentAssertion:
    return mlir::emitError(loc, "unsupported statement: concurrent assertion");
  case slang::ast::StatementKind::DisableFork:
    return mlir::emitError(loc, "unsupported statement: diable fork");
  case slang::ast::StatementKind::Wait:
    return mlir::emitError(loc, "unsupported statement: wait");
  case slang::ast::StatementKind::WaitFork:
    return mlir::emitError(loc, "unsupported statement: wait fork");
  case slang::ast::StatementKind::WaitOrder:
    return mlir::emitError(loc, "unsupported statement: wait order");
  case slang::ast::StatementKind::EventTrigger:
    return mlir::emitError(loc, "unsupported statement: event trigger");
  case slang::ast::StatementKind::ProceduralAssign:
    return mlir::emitError(loc, "unsupported statement: procedural assign");
  case slang::ast::StatementKind::ProceduralDeassign:
    return mlir::emitError(loc, "unsupported statement: procedural deassign");
  case slang::ast::StatementKind::RandCase:
    return mlir::emitError(loc, "unsupported statement: rand case");
  case slang::ast::StatementKind::RandSequence:
    return mlir::emitError(loc, "unsupported statement: rand sequence");
  case slang::ast::StatementKind::Conditional:
    visitConditionalStmt(&statement->as<slang::ast::ConditionalStatement>());
    break;
  default:
    mlir::emitRemark(loc, "unsupported statement");
    return failure();
  }

  return success();
}
