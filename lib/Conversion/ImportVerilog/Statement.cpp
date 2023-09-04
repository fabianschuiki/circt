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

void Context::convertExpression(const slang::ast::Expression *expression) {
  auto loc = convertLocation(expression->sourceRange.start());
  switch (expression->kind) {
  case slang::ast::ExpressionKind::IntegerLiteral:
    assert(0 && "TODO");
    break;
  case slang::ast::ExpressionKind::NamedValue:
    assert(0 && "TODO");
    break;
  case slang::ast::ExpressionKind::UnaryOp:
    assert(0 && "TODO");
    break;
  case slang::ast::ExpressionKind::BinaryOp:
    assert(0 && "TODO");
    break;
  case slang::ast::ExpressionKind::Assignment:
    // I think that needs to call another function, like visitExpr(...), which
    // handles details about expressions.
    assert(0 && "TODO");
    break;
  case slang::ast::ExpressionKind::Conversion:
    assert(0 && "TODO");
    break;
    // Other cases need to be appended.
  default:
    mlir::emitError(loc, "unsupported ExpressionKind");
    break;
  }
}

void Context::convertStatement(const slang::ast::Statement *statement) {
  auto loc = convertLocation(statement->sourceRange.start());
  switch (statement->kind) {
  case slang::ast::StatementKind::List:
    for (auto it = statement->as<slang::ast::StatementList>().list.begin();
         it != statement->as<slang::ast::StatementList>().list.end(); it++) {
      convertStatement(*it);
    }
    break;
  case slang::ast::StatementKind::Block:
    convertStatement(&statement->as<slang::ast::BlockStatement>().body);
    break;
  case slang::ast::StatementKind::ExpressionStatement:
    convertExpression(&statement->as<slang::ast::ExpressionStatement>().expr);
    break;
  case slang::ast::StatementKind::VariableDeclaration:
    assert(0 && "TODO");
    break;
  case slang::ast::StatementKind::Return:
    assert(0 && "TODO");
    break;
  case slang::ast::StatementKind::Break:
    assert(0 && "TODO");
    break;
  case slang::ast::StatementKind::Continue:
    assert(0 && "TODO");
    break;
  case slang::ast::StatementKind::Case:
    assert(0 && "TODO");
    break;
  case slang::ast::StatementKind::PatternCase:
    assert(0 && "TODO");
    break;
  case slang::ast::StatementKind::ForLoop:
    assert(0 && "TODO");
    break;
  case slang::ast::StatementKind::RepeatLoop:
    assert(0 && "TODO");
    break;
  case slang::ast::StatementKind::ForeachLoop:
    assert(0 && "TODO");
    break;
  case slang::ast::StatementKind::WhileLoop:
    assert(0 && "TODO");
    break;
  case slang::ast::StatementKind::DoWhileLoop:
    assert(0 && "TODO");
    break;
  case slang::ast::StatementKind::ForeverLoop:
    assert(0 && "TODO");
    break;
  case slang::ast::StatementKind::Timed:
    assert(0 && "TODO");
    break;
  case slang::ast::StatementKind::ImmediateAssertion:
    assert(0 && "TODO");
    break;
  case slang::ast::StatementKind::ConcurrentAssertion:
    assert(0 && "TODO");
    break;
  case slang::ast::StatementKind::DisableFork:
    assert(0 && "TODO");
    break;
  case slang::ast::StatementKind::Wait:
    assert(0 && "TODO");
    break;
  case slang::ast::StatementKind::WaitFork:
    assert(0 && "TODO");
    break;
  case slang::ast::StatementKind::WaitOrder:
    assert(0 && "TODO");
    break;
  case slang::ast::StatementKind::EventTrigger:
    assert(0 && "TODO");
    break;
  case slang::ast::StatementKind::ProceduralAssign:
    assert(0 && "TODO");
    break;
  case slang::ast::StatementKind::ProceduralDeassign:
    assert(0 && "TODO");
    break;
  case slang::ast::StatementKind::RandCase:
    assert(0 && "TODO");
    break;
  case slang::ast::StatementKind::RandSequence:
    assert(0 && "TODO");
    break;
  case slang::ast::StatementKind::ProceduralChecker:
    assert(0 && "TODO");
    break;
  case slang::ast::StatementKind::Conditional:
    assert(0 && "TODO");
    break;
  default:
    mlir::emitRemark(loc, "unsupported statement");
    break;
  }
}