//===- AST.h - AST for the Tin language -----------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#pragma once
#include "circt/Support/LLVM.h"
#include "mlir/IR/BuiltinAttributes.h"

namespace circt {
namespace tin {
namespace ast {

struct Item;

/// A root node in the AST, corresponding to a parsed file.
struct Root {
  ArrayRef<Item *> items;
};

/// Base class for all items.
struct Item {
  enum class Kind {
#define AST_ITEM(NAME) NAME,
#include "tin/AST.def"
  };

  const Kind kind;
  Location loc;
};

/// A module definition.
struct ModItem : public Item {
  static bool classof(const Item *item) { return item->kind == Kind::Mod; }
  StringAttr name;
};

} // namespace ast

/// A container that holds an entire AST and owns its memory.
struct AST {
  /// A collection of root nodes.
  SmallVector<ast::Root *> roots;

  /// Move a single node into the memory owned by the AST and return a reference
  /// to it. The reference remains valid for as long as the AST is alive.
  template <class C>
  C &create(C &&node) {
    return *new (allocator) C(std::move(node));
  }

  /// Move an array of nodes into the memory owned by the AST and return an
  /// ArrayRef to it. The ArrayRef remains valid for as long as the AST is
  /// alive.
  template <class Container, typename T = typename std::remove_reference<
                                 Container>::type::value_type>
  ArrayRef<T> array(Container &&container) {
    auto num = llvm::range_size(container);
    T *data = allocator.Allocate<T>(num);
    ArrayRef a(data, num);
    for (auto &&element : container)
      new (data++) T(element);
    return a;
  }

private:
  llvm::BumpPtrAllocator allocator;
};

} // namespace tin
} // namespace circt
