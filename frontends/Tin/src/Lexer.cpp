//===- Lexer.cpp - Lexer for the Tin language -----------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "tin/Lexer.h"

#include "mlir/IR/Diagnostics.h"
#include "llvm/Support/ManagedStatic.h"

using namespace circt;
using namespace tin;

//===----------------------------------------------------------------------===//
// TokenKind
//===----------------------------------------------------------------------===//

StringRef tin::symbolizeTokenKind(TokenKind kind) {
  switch (kind) {
  case TokenKind::eof:
    return "end of file";
  case TokenKind::error:
    return "<error>";
  case TokenKind::ident:
    return "identifier";
  case TokenKind::num_lit:
    return "number literal";

#define TOK_KEYWORD(IDENT)                                                     \
  case TokenKind::kw_##IDENT:                                                  \
    return "keyword `" #IDENT "`";

#define TOK_SYMBOL(NAME, SPELLING)                                             \
  case TokenKind::NAME:                                                        \
    return "`" SPELLING "`";

#include "tin/Tokens.def"
  }
  llvm_unreachable("should handle all token kinds");
  return "<unknown>";
}

//===----------------------------------------------------------------------===//
// Lexer
//===----------------------------------------------------------------------===//

// Create a static lookup table for keywords.
namespace {
struct KeywordTableCreator {
  static void *call() {
    auto table = std::make_unique<llvm::StringMap<TokenKind>>();
#define TOK_KEYWORD(IDENT) table->insert({#IDENT, TokenKind::kw_##IDENT});
#include "tin/Tokens.def"
    return table.release();
  }
};
llvm::ManagedStatic<llvm::StringMap<TokenKind>, KeywordTableCreator>
    staticKeywordTable;
} // namespace

Lexer::Lexer(MLIRContext *context, StringRef text, StringAttr filename)
    : context(context), fullText(text), filename(filename), text(text),
      keywordTable(*staticKeywordTable) {}

/// Check whether a character is considered whitespace.
static bool is_space(char c) {
  return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

/// Check whether a character is a valid digit.
static bool is_digit(char c) { return c >= '0' && c <= '9'; }

/// Check whether a character is a valid start of an identifier.
static bool is_ident_start(char c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

/// Check whether a character is valid in an identifier.
static bool is_ident(char c) { return is_ident_start(c) || is_digit(c); }

/// Check whether `text` starts with a valid 1-character symbol token.
static TokenKind match_symbol1(StringRef text) {
  if (text.size() < 1)
    return TokenKind::eof;
  switch (text[0]) {
#define TOK_SYMBOL1(NAME, SPELLING)                                            \
  case SPELLING[0]:                                                            \
    return TokenKind::NAME;
#include "tin/Tokens.def"
  default:
    return TokenKind::eof;
  }
}

/// Check whether `text` starts with a valid 2-character symbol token.
static TokenKind match_symbol2(StringRef text) {
  if (text.size() < 2)
    return TokenKind::eof;
  switch (text[0] | text[1] << 8) {
#define TOK_SYMBOL2(NAME, SPELLING)                                            \
  case SPELLING[0] | SPELLING[1] << 8:                                         \
    return TokenKind::NAME;
#include "tin/Tokens.def"
  default:
    return TokenKind::eof;
  }
}

Token Lexer::next() {
  // Ignore things in front of the next token.
  while (!text.empty()) {
    // Skip whitespace.
    while (!text.empty() && is_space(text[0]))
      text = text.drop_front();

    // Skip single-line comments.
    if (text.consume_front("//")) {
      while (!text.empty() && text[0] != '\n')
        text = text.drop_front();
      continue;
    }

    // Skip multi-line comments.
    auto commentStart = text;
    if (text.consume_front("/*")) {
      while (!text.empty() && !text.starts_with("*/"))
        text = text.drop_front();
      if (!text.consume_front("*/")) {
        mlir::emitError(locationOfSubstring(commentStart),
                        "unclosed comment; missing `*/`");
        return {"", TokenKind::error};
      }
      continue;
    }

    // Nothing left to ignore.
    break;
  }
  if (text.empty())
    return {text, TokenKind::eof};

  // Snapshot the input before we do any lexing for the next token.
  auto initialText = text;

  // Parse symbols.
  if (auto kind = match_symbol2(text.data()); kind != TokenKind::eof) {
    text = text.drop_front(2);
    return {initialText.take_front(2), kind};
  }
  if (auto kind = match_symbol1(text.data()); kind != TokenKind::eof) {
    text = text.drop_front(1);
    return {initialText.take_front(1), kind};
  }

  // Parse identifiers.
  if (is_ident_start(text[0])) {
    auto ident = text.take_while(is_ident);
    text = text.drop_front(ident.size());
    auto kind = TokenKind::ident;
    if (auto it = keywordTable.find(ident); it != keywordTable.end())
      kind = it->second;
    return {ident, kind};
  }

  // Parse number literals.
  if (is_digit(text[0])) {
    auto num = text.take_while(is_ident);
    text = text.drop_front(num.size());
    return {num, TokenKind::num_lit};
  }

  // If we get here we didn't recognize what's in the input text.
  mlir::emitError(locationOfSubstring(initialText.substr(0, 1)),
                  "unknown character `")
      << initialText[0] << "`";
  return {initialText.substr(0, 0), TokenKind::error};
}

Location Lexer::locationOfSubstring(StringRef substring) {
  assert(fullText.begin() <= substring.begin() &&
         fullText.end() >= substring.end() &&
         "substring is not part of the full text");

  // Compute the byte offset of `substring` into the full text being lexed and
  // get a string ref to the full text up to the beginning of `substring`,
  // basically everything we've lexed so far.
  auto offset = substring.begin() - fullText.begin();
  auto line = fullText.substr(0, offset);

  // Count the number of lines we've lexed and leave only the current line in
  // the string ref. Its length indicates the column at which we currently are.
  unsigned lineNum = 1;
  size_t pos = 0;
  while ((pos = line.find('\n')) != StringRef::npos) {
    ++lineNum;
    ++pos;
    if (pos < line.size() && line[pos] == '\r')
      ++pos;
    line = line.substr(pos);
  }

  return FileLineColLoc::get(context, filename, lineNum, line.size() + 1);
}
