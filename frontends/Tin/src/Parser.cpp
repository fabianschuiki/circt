//===- Parser.cpp - Parser for the Tin language ---------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "tin/Parser.h"

#include "mlir/IR/Diagnostics.h"

using namespace circt;
using namespace tin;

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
    while (notAtDelimiter(TokenKind::rcurly))
      if (failed(parseStatement()))
        return {};
    if (!require(TokenKind::rcurly))
      return {};

    return &ast.create<ast::ModItem>(
        {{ast::Item::Kind::Mod, loc(name)},
         StringAttr::get(lexer.context, name.spelling)});
  }

  mlir::emitError(loc(), "expected item, found ") << token;
  return {};
}

LogicalResult Parser::parseStatement() {
  return mlir::emitError(loc(), "expected statement, found ") << token;
}
