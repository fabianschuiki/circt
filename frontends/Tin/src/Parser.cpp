//===- Parser.cpp - Parser for the Tin language ---------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "tin/Parser.h"

#include "mlir/IR/Diagnostics.h"
#include "llvm/ADT/SmallString.h"

using namespace circt;
using namespace tin;

static bool isValidDigitForBase(char c, int base) {
  if (c >= '0' && c <= '9')
    return (c - '0') < base;
  c = std::tolower(c);
  if (c >= 'a' && c <= 'f')
    return (c - 'a' + 10) < base;
  return false;
}

static std::optional<ast::UnaryOp> isUnaryOp(TokenKind kind) {
  switch (kind) {
#define AST_UNARY(NAME, TOKEN)                                                 \
  case TokenKind::TOKEN:                                                       \
    return ast::UnaryOp::NAME;
#include "tin/AST.def"
  default:
    return {};
  }
}

static std::optional<ast::BinaryOp> isBinaryOp(TokenKind kind) {
  switch (kind) {
#define AST_BINARY(NAME, TOKEN, PREC)                                          \
  case TokenKind::TOKEN:                                                       \
    return ast::BinaryOp::NAME;
#include "tin/AST.def"
  default:
    return {};
  }
}

//===----------------------------------------------------------------------===//
// Parser
//===----------------------------------------------------------------------===//

Parser::Parser(Lexer &lexer, AST &ast) : lexer(lexer), ast(ast) {
  token = lexer.next();
}

Location Parser::loc() { return loc(token); }

Location Parser::loc(const Token &token) {
  return lexer.locationOfSubstring(token.spelling);
}

Token Parser::consume() {
  auto consumedToken = token;
  token = lexer.next();
  return consumedToken;
}

Token Parser::consumeIf(TokenKind kind) {
  if (isa(kind))
    return consume();
  return {token.spelling.substr(0, 0), TokenKind::Eof};
}

[[nodiscard]]
Token Parser::require(TokenKind kind, const Twine &msg) {
  if (isa(kind))
    return consume();
  auto d = mlir::emitError(loc(), "expected ");
  if (msg.isTriviallyEmpty())
    d << symbolizeTokenKind(kind);
  else
    d << msg;
  d << ", found " << token;
  return {token.spelling.substr(0, 0), TokenKind::Eof};
}

//===----------------------------------------------------------------------===//
// Grammar
//===----------------------------------------------------------------------===//

ast::Root *Parser::parseRoot() {
  SmallVector<ast::Item *> items;
  while (token) {
    auto item = parseItem();
    if (!item)
      return {};
    items.push_back(item);
  }

  return &ast.create<ast::Root>({ast.array(items)});
}

ast::Item *Parser::parseItem() {
  // Parse module definitions.
  if (auto kw = consumeIf(TokenKind::Kw_mod)) {
    auto name = require(TokenKind::Ident, "module name");
    if (!name)
      return {};

    // Parse the ports.
    if (!require(TokenKind::LParen))
      return {};
    if (!require(TokenKind::RParen))
      return {};

    // Parse the body.
    if (!require(TokenKind::LCurly))
      return {};
    SmallVector<ast::Stmt *> stmts;
    while (notAtDelimiter(TokenKind::RCurly)) {
      auto *stmt = parseStmt();
      if (!stmt)
        return {};
      stmts.push_back(stmt);
    }
    if (!require(TokenKind::RCurly))
      return {};

    return &ast.create<ast::ModItem>(
        {{ast::Item::Kind::Mod, loc(name)},
         StringAttr::get(lexer.context, name.spelling),
         ast.array(stmts)});
  }

  mlir::emitError(loc(), "expected item, found ") << token;
  return {};
}

PointerUnion<ast::Stmt *, ast::Expr *> Parser::parseStmtOrExpr() {
  // Ignore stray semicolons.
  if (auto token = consumeIf(TokenKind::Semicolon))
    return &ast.create<ast::EmptyStmt>({{ast::Stmt::Kind::Empty, loc(token)}});

  // Otherwise this is a statement that starts with an expression.
  return parseExpr();
}

ast::Stmt *Parser::parseStmt() {
  auto node = parseStmtOrExpr();
  if (!node)
    return {};

  // If we've parsed a statement, return it.
  if (auto *stmt = dyn_cast<ast::Stmt *>(node))
    return stmt;
  auto *expr = cast<ast::Expr *>(node);

  // Otherwise we've parsed an expression, also parse the subsequent semicolon
  // if the expression requires one. Some expressions, like `{...}` don't need a
  // semicolon.
  if (!require(TokenKind::Semicolon))
    return {};

  return &ast.create<ast::ExprStmt>({{ast::Stmt::Kind::Expr, expr->loc}, expr});
}

static std::optional<unsigned> consumeWidthSuffix(StringRef &spelling) {
  // Seek over the trailing digits.
  unsigned pos = spelling.size();
  auto is_digit = [](char c) { return c >= '0' && c <= '9'; };
  while (pos > 0 && is_digit(spelling[pos - 1]))
    --pos;
  if (pos == 0 || pos == spelling.size())
    return {};

  // Consume `i[0-9]+`.
  if (spelling[pos - 1] == 'i') {
    unsigned width;
    assert(!spelling.substr(pos).getAsInteger(10, width));
    spelling = spelling.substr(0, pos - 1);
    return width;
  }

  return {};
}

ast::Expr *Parser::parseExpr(ast::Precedence minPrec) {
  auto *expr = parsePrimaryExpr();
  if (!expr)
    return {};
  return parseInfixExpr(expr, minPrec);
}

ast::Expr *Parser::parsePrimaryExpr() {
  // Parse number literals.
  if (auto lit = consumeIf(TokenKind::NumLit)) {
    auto spelling = lit.spelling;

    // Handle the optional `i[0-9]+` type suffix.
    auto width = consumeWidthSuffix(spelling);
    auto spellingWithoutSuffix = spelling;

    // Determine the base.
    unsigned base = 10;
    if (spelling.consume_front("0b"))
      base = 2;
    else if (spelling.consume_front("0o"))
      base = 8;
    else if (spelling.consume_front("0x"))
      base = 16;

    // Filter out `_` and check for invalid digits for the given base.
    SmallString<32> digits;
    digits.reserve(spelling.size());
    for (unsigned i = 0, e = spelling.size(); i != e; ++i) {
      if (spelling[i] == '_')
        continue;
      if (!isValidDigitForBase(spelling[i], base)) {
        mlir::emitError(lexer.locationOfSubstring(spelling.substr(i)))
            << "`" << spelling[i] << "` is not a valid base-" << base
            << " digit";
        return {};
      }
      digits.push_back(spelling[i]);
    }
    if (digits.empty()) {
      mlir::emitError(loc(lit), "number literal has no digits");
      return {};
    }

    // Parse the integer.
    APInt value;
    assert(!digits.str().getAsInteger(base, value));

    // Resize to the explicit width.
    if (width.has_value()) {
      if (value.getActiveBits() > *width) {
        mlir::emitError(loc(lit))
            << "integer `" << spellingWithoutSuffix << "` does not fit into "
            << *width << " bits";
        return {};
      }
      value = value.zextOrTrunc(*width);
    } else {
      value = value.zextOrTrunc(value.getActiveBits());
    }

    return &ast.create<ast::NumLitExpr>(
        {{ast::Expr::Kind::NumLit, loc(lit)}, value});
  }

  // Parse parenthesized expressions.
  if (auto lparen = consumeIf(TokenKind::LParen)) {
    auto *expr = parseExpr();
    if (!expr)
      return {};
    if (!require(TokenKind::RParen))
      return {};
    return &ast.create<ast::ParenExpr>(
        {{ast::Expr::Kind::Paren, loc(lparen)}, expr});
  }

  // Parse unary operators.
  if (auto op = isUnaryOp(token.kind)) {
    auto opToken = consume();
    auto *arg = parsePrimaryExpr();
    if (!arg)
      return {};
    return &ast.create<ast::UnaryExpr>(
        {{ast::Expr::Kind::Unary, loc(opToken)}, *op, arg});
  }

  mlir::emitError(loc(), "expected expression, found ") << token;
  return {};
}

ast::Expr *Parser::parseInfixExpr(ast::Expr *expr, ast::Precedence minPrec) {
  while (true) {
    // Handle binary operators.
    auto op = isBinaryOp(token.kind);
    if (!op)
      return expr;

    // If this operator's precedence is below the minimum precedence, return.
    // This ensures that a `*` does not gobble up a `+`.
    auto opPrec = getPrecedence(*op);
    if (opPrec < minPrec)
      return expr;

    // Consume the operator and parse the right-hand side expression.
    auto opToken = consume();
    auto *rhs = parseExpr(opPrec);

    // Form the new left-hand side.
    expr = &ast.create<ast::BinaryExpr>(
        {{ast::Expr::Kind::Binary, loc(opToken)}, *op, expr, rhs});
  }
}
