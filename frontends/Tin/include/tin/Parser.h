//===- Parser.h - Parser for the Tin language -----------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#pragma once
#include "tin/Lexer.h"

#include "circt/Support/LLVM.h"
#include "mlir/IR/BuiltinAttributes.h"

namespace circt {
namespace tin {

class Parser {
public:
  Parser(Lexer &lexer);

  LogicalResult parseRoot();
  LogicalResult parseItem();
  LogicalResult parseStatement();

  Lexer &lexer;

private:
  Token token;

  Location loc();
  Token consume();
  Token consumeIf(TokenKind kind);
  Token require(TokenKind kind, const Twine &msg = {});
  bool isa(TokenKind kind) { return token.kind == kind; }
  bool notAtDelimiter(TokenKind kind) { return token && token.kind != kind; }
};

} // namespace tin
} // namespace circt
