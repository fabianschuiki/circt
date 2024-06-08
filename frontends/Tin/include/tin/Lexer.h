//===- Lexer.h - Lexer for the Tin language -------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#pragma once
#include "circt/Support/LLVM.h"
#include "mlir/IR/BuiltinAttributes.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/Support/SourceMgr.h"

namespace circt {
namespace tin {

enum class TokenKind {
  Eof,
  Error,
#define TOK_ANY(NAME) NAME,
#include "tin/Tokens.def"
};

StringRef symbolizeTokenKind(TokenKind kind);

struct Token {
  StringRef spelling;
  TokenKind kind;

  /// Check whether the token represents the end of the input file.
  explicit operator bool() const { return kind != TokenKind::Eof; }
};

// Allow `Token` to be printed.
template <typename T>
static T &operator<<(T &os, const Token &token) {
  os << symbolizeTokenKind(token.kind);
  if (token.kind == TokenKind::Ident || token.kind == TokenKind::NumLit)
    os << " `" << token.spelling << "`";
  return os;
}

class Lexer {
public:
  Lexer(MLIRContext *context, llvm::SourceMgr &sourceMgr);
  Token next();
  Location locationOfSubstring(StringRef substring);

  MLIRContext *context;
  llvm::SourceMgr &sourceMgr;
  StringAttr filename;

private:
  /// The remaining text to be tokenized.
  StringRef text;

  /// A reference to a statically-allocated lookup table for keywords.
  llvm::StringMap<TokenKind> &keywordTable;
};

} // namespace tin
} // namespace circt
