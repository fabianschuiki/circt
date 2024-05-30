//===- AST.h - AST for the Tin language -----------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#pragma once
#include "circt/Support/LLVM.h"

namespace circt {
namespace tin {
namespace ast {

/// Base class for all AST nodes.
struct Node {
  /// The location to report to the user for diagnostics on this AST node.
  Location loc;
};

/// Base class for all items.
struct Item : public Node {};

/// A module definition.
struct ModItem : public Item {};

} // namespace ast
} // namespace tin
} // namespace circt
