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

Parser::Parser(Lexer &lexer) : lexer(lexer) { token = lexer.next(); }

Location Parser::loc() { return lexer.locationOfSubstring(token.spelling); }

Token Parser::consume() {
  auto consumedToken = token;
  token = lexer.next();
  // lastloc = x;
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

LogicalResult Parser::parseRoot() {
  while (token)
    if (failed(parseItem()))
      return failure();
  return success();
}

LogicalResult Parser::parseItem() {
  // Parse module definitions.
  if (auto kw = consumeIf(TokenKind::kw_mod)) {
    auto name = require(TokenKind::ident, "module name");
    require(TokenKind::lparen);
    require(TokenKind::rparen);
    require(TokenKind::lcurly);
    while (notAtDelimiter(TokenKind::rcurly))
      if (failed(parseStatement()))
        return failure();
    require(TokenKind::rcurly);
    llvm::errs() << "found a module named " << name.spelling << "!\n";
    return success();
  }

  // llvm::errs() << token.spelling << "\n";
  return mlir::emitError(loc(), "expected item, found ") << token;
}

LogicalResult Parser::parseStatement() {
  return mlir::emitError(loc(), "expected statement, found ") << token;
}
