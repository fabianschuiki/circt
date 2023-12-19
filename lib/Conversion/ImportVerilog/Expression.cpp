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
    const slang::ast::IntegerLiteral *integerLiteralExpr) {
  auto srcValue = integerLiteralExpr->getValue().as<uint32_t>().value();
  return builder.create<moore::ConstantOp>(
      convertLocation(integerLiteralExpr->sourceRange.start()),
      convertType(*integerLiteralExpr->type), srcValue);
}

// Detail processing about named value.
Value Context::visitNamedValue(
    const slang::ast::NamedValueExpression *namedValueExpr) {
  return varSymbolTable.lookup(namedValueExpr->getSymbolReference()->name);
}

// Detail processing about unary expression.
Value Context::visitUnaryOp(const slang::ast::UnaryExpression *unaryExpr) {
  auto loc = convertLocation(unaryExpr->sourceRange.start());
  auto value = visitExpression(&unaryExpr->operand());

  switch (unaryExpr->op) {
  case slang::ast::UnaryOperator::Plus:
    return builder.create<moore::UnaryOp>(loc, moore::Unary::Plus, value);
  case slang::ast::UnaryOperator::Minus:
    return builder.create<moore::UnaryOp>(loc, moore::Unary::Minus, value);
  case slang::ast::UnaryOperator::BitwiseNot:
    return builder.create<moore::ReductionOp>(loc, moore::Reduction::BitwiseNot,
                                              value);
  case slang::ast::UnaryOperator::BitwiseAnd:
    return builder.create<moore::ReductionOp>(loc, moore::Reduction::BitwiseAnd,
                                              value);
  case slang::ast::UnaryOperator::BitwiseOr:
    return builder.create<moore::ReductionOp>(loc, moore::Reduction::BitwiseOr,
                                              value);
  case slang::ast::UnaryOperator::BitwiseXor:
    return builder.create<moore::ReductionOp>(loc, moore::Reduction::BitwiseXor,
                                              value);
  case slang::ast::UnaryOperator::BitwiseNand:
    return builder.create<moore::ReductionOp>(
        loc, moore::Reduction::BitwiseNand, value);
  case slang::ast::UnaryOperator::BitwiseNor:
    return builder.create<moore::ReductionOp>(loc, moore::Reduction::BitwiseNor,
                                              value);
  case slang::ast::UnaryOperator::BitwiseXnor:
    return builder.create<moore::ReductionOp>(
        loc, moore::Reduction::BitwiseXnor, value);
  case slang::ast::UnaryOperator::LogicalNot:
    return builder.create<moore::UnaryOp>(loc, moore::Unary::LogicalNot, value);
  case slang::ast::UnaryOperator::Preincrement:
  case slang::ast::UnaryOperator::Predecrement:
  case slang::ast::UnaryOperator::Postincrement:
  case slang::ast::UnaryOperator::Postdecrement:
  default:
    mlir::emitError(loc, "unsupported unary operator");
    return nullptr;
  }
  return nullptr;
}

// Detail processing about binary expression.
Value Context::visitBinaryOp(const slang::ast::BinaryExpression *binaryExpr) {
  auto loc = convertLocation(binaryExpr->sourceRange.start());
  Value lhs = visitExpression(&binaryExpr->left());
  if (!lhs)
    return nullptr;
  Value rhs = visitExpression(&binaryExpr->right());
  if (!rhs)
    return nullptr;

  switch (binaryExpr->op) {
  case slang::ast::BinaryOperator::Add:
    return builder.create<moore::AddOp>(loc, lhs, rhs);
  case slang::ast::BinaryOperator::Subtract:
    mlir::emitError(loc, "unsupported binary operator : subtract");
    return nullptr;
  case slang::ast::BinaryOperator::Multiply:
    return builder.create<moore::MulOp>(loc, lhs, rhs);
  case slang::ast::BinaryOperator::Divide:
    mlir::emitError(loc, "unsupported binary operator : divide");
    return nullptr;
  case slang::ast::BinaryOperator::Mod:
    mlir::emitError(loc, "unsupported binary operator : mod");
    return nullptr;
  case slang::ast::BinaryOperator::BinaryAnd:
    return builder.create<moore::BitwiseOp>(loc, moore::Bitwise::BinaryAnd, lhs,
                                            rhs);
  case slang::ast::BinaryOperator::BinaryOr:
    return builder.create<moore::BitwiseOp>(loc, moore::Bitwise::BinaryOr, lhs,
                                            rhs);
  case slang::ast::BinaryOperator::BinaryXor:
    return builder.create<moore::BitwiseOp>(loc, moore::Bitwise::BinaryXor, lhs,
                                            rhs);
  case slang::ast::BinaryOperator::BinaryXnor:
    return builder.create<moore::BitwiseOp>(loc, moore::Bitwise::BinaryXnor,
                                            lhs, rhs);
  case slang::ast::BinaryOperator::Equality:
    return builder.create<moore::EqualityOp>(loc, lhs, rhs);
  case slang::ast::BinaryOperator::Inequality:
    return builder.create<moore::InEqualityOp>(loc, lhs, rhs);
  case slang::ast::BinaryOperator::CaseEquality:
    return builder.create<moore::EqualityOp>(loc, lhs, rhs,
                                             builder.getUnitAttr());
  case slang::ast::BinaryOperator::CaseInequality:
    return builder.create<moore::InEqualityOp>(loc, lhs, rhs,
                                               builder.getUnitAttr());
  case slang::ast::BinaryOperator::GreaterThanEqual:
    // TODO: I think should integrate these four relation operators into one
    // builder.create. But I failed, the error is `resultNumber <
    // getNumResults() && ... ` from Operation.h:983.
    return builder.create<moore::RelationalOp>(
        loc, moore::Relation::GreaterThanEqual, lhs, rhs);
  case slang::ast::BinaryOperator::GreaterThan:
    return builder.create<moore::RelationalOp>(
        loc, moore::Relation::GreaterThan, lhs, rhs);
  case slang::ast::BinaryOperator::LessThanEqual:
    return builder.create<moore::RelationalOp>(
        loc, moore::Relation::LessThanEqual, lhs, rhs);
  case slang::ast::BinaryOperator::LessThan:
    return builder.create<moore::RelationalOp>(loc, moore::Relation::LessThan,
                                               lhs, rhs);
  case slang::ast::BinaryOperator::WildcardEquality:
    mlir::emitError(loc, "unsupported binary operator : wildcard equality");
    return nullptr;
  case slang::ast::BinaryOperator::WildcardInequality:
    mlir::emitError(loc, "unsupported binary operator : wildcard inequality");
    return nullptr;
  case slang::ast::BinaryOperator::LogicalAnd:
    return builder.create<moore::LogicalOp>(loc, moore::Logic::LogicalAnd, lhs,
                                            rhs);
  case slang::ast::BinaryOperator::LogicalOr:
    return builder.create<moore::LogicalOp>(loc, moore::Logic::LogicalOr, lhs,
                                            rhs);
  case slang::ast::BinaryOperator::LogicalImplication:
    return builder.create<moore::LogicalOp>(
        loc, moore::Logic::LogicalImplication, lhs, rhs);
  case slang::ast::BinaryOperator::LogicalEquivalence:
    return builder.create<moore::LogicalOp>(
        loc, moore::Logic::LogicalEquivalence, lhs, rhs);
  case slang::ast::BinaryOperator::LogicalShiftLeft:
    return builder.create<moore::ShlOp>(loc, lhs, rhs);
  case slang::ast::BinaryOperator::LogicalShiftRight:
    return builder.create<moore::ShrOp>(loc, lhs, rhs);
  case slang::ast::BinaryOperator::ArithmeticShiftLeft:
    return builder.create<moore::ShlOp>(loc, lhs, rhs, builder.getUnitAttr());
  case slang::ast::BinaryOperator::ArithmeticShiftRight:
    return builder.create<moore::ShrOp>(loc, lhs, rhs, builder.getUnitAttr());
  case slang::ast::BinaryOperator::Power:
    mlir::emitError(loc, "unsupported binary operator : power");
    return nullptr;
  default:
    mlir::emitError(loc, "unsupported binary operator");
    return nullptr;
  }
  return nullptr;
}

// Detail processing about assignment.
Value Context::visitAssignmentExpr(
    const slang::ast::AssignmentExpression *assignmentExpr) {
  auto loc = convertLocation(assignmentExpr->sourceRange.start());
  Value lhs = visitExpression(&assignmentExpr->left());
  if (!lhs)
    return nullptr;
  Value rhs = visitExpression(&assignmentExpr->right());
  if (!rhs)
    return nullptr;
  if (lhs.getType() != rhs.getType())
    rhs = builder.create<moore::ConversionOp>(loc, lhs.getType(), rhs);
  if (assignmentExpr->isNonBlocking())
    builder.create<moore::PAssignOp>(loc, lhs, rhs);
  else {
    if (assignmentExpr->syntax->parent->kind ==
        slang::syntax::SyntaxKind::ContinuousAssign)
      builder.create<moore::CAssignOp>(loc, lhs, rhs);
    else if (assignmentExpr->syntax->parent->kind ==
             slang::syntax::SyntaxKind::ProceduralAssignStatement)
      builder.create<moore::PCAssignOp>(loc, lhs, rhs);
    else
      builder.create<moore::BPAssignOp>(loc, lhs, rhs);
  }
  return lhs;
}

// Detail processing about concatenation.
Value Context::visitConcatenation(
    const slang::ast::ConcatenationExpression *concatExpr) {
  auto loc = convertLocation(concatExpr->sourceRange.start());
  SmallVector<Value> operands;
  for (auto *operand : concatExpr->operands()) {
    operands.push_back(visitExpression(operand));
  }
  return builder.create<moore::ConcatOp>(loc, operands);
}

// It can handle the expressions like literal, assignment, conversion, and etc,
// which can be reviewed in slang/include/slang/ast/ASTVisitor.h.
Value Context::visitExpression(const slang::ast::Expression *expression) {
  auto loc = convertLocation(expression->sourceRange.start());
  switch (expression->kind) {
  case slang::ast::ExpressionKind::IntegerLiteral:
    return visitIntegerLiteral(&expression->as<slang::ast::IntegerLiteral>());
  case slang::ast::ExpressionKind::NamedValue:
    return visitNamedValue(&expression->as<slang::ast::NamedValueExpression>());
  case slang::ast::ExpressionKind::UnaryOp:
    return visitUnaryOp(&expression->as<slang::ast::UnaryExpression>());
  case slang::ast::ExpressionKind::BinaryOp:
    return visitBinaryOp(&expression->as<slang::ast::BinaryExpression>());
  case slang::ast::ExpressionKind::Assignment:
    return visitAssignmentExpr(
        &expression->as<slang::ast::AssignmentExpression>());
  case slang::ast::ExpressionKind::Concatenation:
    return visitConcatenation(
        &expression->as<slang::ast::ConcatenationExpression>());
  case slang::ast::ExpressionKind::Conversion:
    return visitExpression(
        &expression->as<slang::ast::ConversionExpression>().operand());
  // There is other cases.
  default:
    mlir::emitError(loc, "unsupported expression");
    return nullptr;
  }

  return nullptr;
}
