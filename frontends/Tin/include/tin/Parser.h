//===- Parser.h - Parser for the Tin language -----------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#pragma once
#include "tin/AST.h"
#include "tin/Lexer.h"

#include "circt/Support/LLVM.h"
#include "mlir/IR/BuiltinAttributes.h"

namespace circt {
namespace tin {

class Parser {
public:
  Parser(Lexer &lexer, AST &ast);

  ast::Root *parseRoot();
  ast::Item *parseItem();
  PointerUnion<ast::Stmt *, ast::Expr *> parseStmtOrExpr();
  ast::Stmt *parseStmt();
  ast::Expr *parseExpr(ast::Precedence minPrec = ast::Precedence::Min);
  ast::Expr *parsePrimaryExpr();
  ast::Expr *parseInfixExpr(ast::Expr *expr, ast::Precedence minPrec);

  Lexer &lexer;
  AST &ast;

private:
  Token token;

  Location loc();
  Location loc(const Token &token);
  Token consume();
  Token consumeIf(TokenKind kind);
  Token require(TokenKind kind, const Twine &msg = {});
  bool isa(TokenKind kind) { return token.kind == kind; }
  bool notAtDelimiter(TokenKind kind) { return token && token.kind != kind; }
};

} // namespace tin
} // namespace circt
