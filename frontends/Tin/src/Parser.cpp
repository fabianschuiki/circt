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
  return {token.spelling.substr(0, 0), TokenKind::eof};
}

Token Parser::require(TokenKind kind, const Twine &msg) {
  if (isa(kind))
    return consume();
  auto d = mlir::emitError(loc(), "expected ");
  if (msg.isTriviallyEmpty())
    d << symbolizeTokenKind(kind);
  else
    d << msg;
  d << ", found " << token;
  return {token.spelling.substr(0, 0), TokenKind::eof};
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
  if (auto kw = consumeIf(TokenKind::kw_mod)) {
    auto name = require(TokenKind::ident, "module name");

    // Parse the ports.
    if (!require(TokenKind::lparen))
      return {};
    if (!require(TokenKind::rparen))
      return {};

    // Parse the body.
    if (!require(TokenKind::lcurly))
      return {};
    SmallVector<ast::Stmt *> stmts;
    while (notAtDelimiter(TokenKind::rcurly)) {
      auto *stmt = parseStmt();
      if (!stmt)
        return {};
      stmts.push_back(stmt);
    }
    if (!require(TokenKind::rcurly))
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
  if (auto token = consumeIf(TokenKind::semicolon))
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
  if (!require(TokenKind::semicolon))
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

ast::Expr *Parser::parseExpr() { return parsePrefixExpr(); }

ast::Expr *Parser::parsePrimaryExpr() {
  // Parse number literals.
  if (auto lit = consumeIf(TokenKind::num_lit)) {
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
  if (auto lparen = consumeIf(TokenKind::lparen)) {
    auto *expr = parseExpr();
    if (!expr)
      return {};
    require(TokenKind::rparen);
    return &ast.create<ast::ParenExpr>(
        {{ast::Expr::Kind::Paren, loc(lparen)}, expr});
  }

  mlir::emitError(loc(), "expected expression, found ") << token;
  return {};
}

ast::Expr *Parser::parsePrefixExpr() {
  // Parse unary operators.
  auto parseUnary = [&](ast::UnaryOp op) -> ast::Expr * {
    auto opToken = consume();
    auto *arg = parsePrefixExpr();
    if (!arg)
      return {};
    return &ast.create<ast::UnaryExpr>(
        {{ast::Expr::Kind::Unary, loc(opToken)}, op, arg});
  };

  switch (token.kind) {
#define AST_UNARY(NAME, TOKEN)                                                 \
  case TokenKind::TOKEN:                                                       \
    return parseUnary(ast::UnaryOp::NAME);
#include "tin/AST.def"
  default:
    break;
  }

  // Otherwise parse a primary expression.
  return parsePrimaryExpr();
}
