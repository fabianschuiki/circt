//===- AST.cpp - AST for the Tin language ---------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "tin/AST.h"

using namespace circt;
using namespace tin;
using namespace ast;

/// Return the precedence of the given binary operator.
Precedence ast::getPrecedence(BinaryOp op) {
  switch (op) {
#define AST_BINARY(NAME, TOKEN, PREC)                                          \
  case BinaryOp::NAME:                                                         \
    return Precedence::P_##PREC;
#include "tin/AST.def"
  };
}
