//===- Statement.cpp - Slang statement conversion -------------------------===//
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

  Value cond = convertExpression(*conditionalStmt->conditions.begin()->expr);
  if (!cond)
    return failure();
  cond = builder.create<moore::BoolCastOp>(loc, cond);
  cond = builder.create<moore::ConversionOp>(loc, builder.getI1Type(), cond);
  // TODO: The above should probably be a `moore.bit_to_i1` op.

  auto ifOp = builder.create<mlir::scf::IfOp>(
      loc, cond, conditionalStmt->ifFalse != nullptr);
  OpBuilder::InsertionGuard guard(builder);

  builder.setInsertionPoint(ifOp.thenYield());
  if (failed(convertStatement(&conditionalStmt->ifTrue)))
    return failure();

  if (conditionalStmt->ifFalse) {
    builder.setInsertionPoint(ifOp.elseYield());
    if (failed(convertStatement(conditionalStmt->ifFalse)))
      return failure();
  }

  return success();
}

// It can handle the statements like case, conditional(if), for loop, and etc.
LogicalResult
Context::convertStatement(const slang::ast::Statement *statement) {
  auto loc = convertLocation(statement->sourceRange.start());
  switch (statement->kind) {
  case slang::ast::StatementKind::Empty:
    return success();
  case slang::ast::StatementKind::List:
    for (auto *stmt : statement->as<slang::ast::StatementList>().list)
      if (failed(convertStatement(stmt)))
        return failure();
    break;
  case slang::ast::StatementKind::Block:
    return convertStatement(&statement->as<slang::ast::BlockStatement>().body);
  case slang::ast::StatementKind::ExpressionStatement:
    return success(convertExpression(
        statement->as<slang::ast::ExpressionStatement>().expr));
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
    if (failed(visitTimingControl(
            &statement->as<slang::ast::TimedStatement>().timing)))
      return failure();
    if (failed(convertStatement(
            &statement->as<slang::ast::TimedStatement>().stmt)))
      return failure();
    break;
  case slang::ast::StatementKind::ImmediateAssertion:
    return mlir::emitError(loc, "unsupported statement: immediate assertion");
  case slang::ast::StatementKind::ConcurrentAssertion:
    return mlir::emitError(loc, "unsupported statement: concurrent assertion");
  case slang::ast::StatementKind::DisableFork:
    return mlir::emitError(loc, "unsupported statement: disable fork");
  case slang::ast::StatementKind::Wait:
    return mlir::emitError(loc, "unsupported statement: wait");
  case slang::ast::StatementKind::WaitFork:
    return mlir::emitError(loc, "unsupported statement: wait fork");
  case slang::ast::StatementKind::WaitOrder:
    return mlir::emitError(loc, "unsupported statement: wait order");
  case slang::ast::StatementKind::EventTrigger:
    return mlir::emitError(loc, "unsupported statement: event trigger");
  case slang::ast::StatementKind::ProceduralAssign:
    return success(convertExpression(
        statement->as<slang::ast::ProceduralAssignStatement>().assignment));
  case slang::ast::StatementKind::ProceduralDeassign:
    return mlir::emitError(loc, "unsupported statement: procedural deassign");
  case slang::ast::StatementKind::RandCase:
    return mlir::emitError(loc, "unsupported statement: rand case");
  case slang::ast::StatementKind::RandSequence:
    return mlir::emitError(loc, "unsupported statement: rand sequence");
  case slang::ast::StatementKind::Conditional:
    return visitConditionalStmt(
        &statement->as<slang::ast::ConditionalStatement>());
  default:
    mlir::emitRemark(loc, "unsupported statement: ")
        << slang::ast::toString(statement->kind);
    return failure();
  }

  return success();
}
