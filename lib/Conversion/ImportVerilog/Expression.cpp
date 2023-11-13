//===- Statement.cpp - Slang expression conversion-------------------------===//
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

// Detail processing about integer literal.
Value Context::visitIntegerLiteral(
    const slang::ast::IntegerLiteral *integerLiteralExpr,
    const slang::ast::Type &type) {
  auto srcValue = integerLiteralExpr->getValue().as<uint32_t>().value();
  return rootBuilder.create<moore::ConstantOp>(
      convertLocation(integerLiteralExpr->sourceRange.start()),
      convertType(type), srcValue);
}

// Detail processing about named value.
Value Context::visitNamedValue(
    const slang::ast::NamedValueExpression *namedValueExpr,
    const slang::ast::Type &type) {
  // TODO:
  return nullptr;
}

// Detail processing about assignment.
Value Context::visitAssignmentExpr(
    const slang::ast::AssignmentExpression *assignmentExpr,
    const slang::ast::Type &type) {
  auto loc = convertLocation(assignmentExpr->sourceRange.start());
  Value lhs = visitExpression(&assignmentExpr->left(), type);
  if (!lhs)
    return nullptr;
  Value rhs = visitExpression(&assignmentExpr->right(), type);
  if (!rhs)
    return nullptr;
  // TODO:
  if (assignmentExpr->right().as_if<slang::ast::NamedValueExpression>()) {
    mlir::emitError(loc, "unsupported assignment of kind like a = b");
    return nullptr;
  }

  /// It is used to choose which assignOp will be used. I think we should
  /// quickly determine the type of assignOp so that implement it. Maybe
  /// 'AnyType' is a decent choice at this stage, though it is unsafe.

  // if (assignmentExpr->isNonBlocking())
  //   rootBuilder.create<moore::PAssignOp>(loc, lhs, rhs);
  // else {
  //   if (assignmentExpr->syntax->parent->kind ==
  //       slang::syntax::SyntaxKind::ContinuousAssign)
  rootBuilder.create<moore::AssignOp>(loc, lhs, rhs);
  //   else
  //     rootBuilder.create<moore::BPAssignOp>(loc, lhs, rhs);
  // }
  return nullptr;
}

// Detail processing about conversion
Value Context::visitConversion(
    const slang::ast::ConversionExpression *conversionExpr,
    const slang::ast::Type &type) {
  auto loc = convertLocation(conversionExpr->sourceRange.start());
  switch (conversionExpr->operand().kind) {
  case slang::ast::ExpressionKind::IntegerLiteral:
    return visitIntegerLiteral(
        &conversionExpr->operand().as<slang::ast::IntegerLiteral>(), type);
  case slang::ast::ExpressionKind::NamedValue:
    return visitNamedValue(
        &conversionExpr->operand().as<slang::ast::NamedValueExpression>(),
        type);
  case slang::ast::ExpressionKind::BinaryOp:
    mlir::emitError(loc, "unsupported conversion expression: binary operator");
    return nullptr;
  case slang::ast::ExpressionKind::ConditionalOp:
    mlir::emitError(loc,
                    "unsupported conversion expression: conditional operator");
    return nullptr;
  case slang::ast::ExpressionKind::Conversion:
    return visitConversion(
        &conversionExpr->operand().as<slang::ast::ConversionExpression>(),
        type);
  case slang::ast::ExpressionKind::LValueReference:
    mlir::emitError(loc, "unsupported conversion expression: lValue reference");
    return nullptr;
  // There is other cases.
  default:
    mlir::emitError(loc, "unsupported conversion expression");
    return nullptr;
  }

  return nullptr;
}

// It can handle the expressions like literal, assignment, conversion, and etc,
// which can be reviewed in slang/include/slang/ast/ASTVisitor.h.
Value Context::visitExpression(const slang::ast::Expression *expression,
                               const slang::ast::Type &type) {
  auto loc = convertLocation(expression->sourceRange.start());
  switch (expression->kind) {
  case slang::ast::ExpressionKind::IntegerLiteral:
    return visitIntegerLiteral(&expression->as<slang::ast::IntegerLiteral>(),
                               type);
  case slang::ast::ExpressionKind::NamedValue:
    return visitNamedValue(&expression->as<slang::ast::NamedValueExpression>(),
                           type);
  case slang::ast::ExpressionKind::Assignment:
    return visitAssignmentExpr(
        &expression->as<slang::ast::AssignmentExpression>(), type);
  case slang::ast::ExpressionKind::Conversion:
    return visitConversion(&expression->as<slang::ast::ConversionExpression>(),
                           type);
  // There is other cases.
  default:
    mlir::emitError(loc, "unsupported expression");
    return nullptr;
  }

  return nullptr;
}
