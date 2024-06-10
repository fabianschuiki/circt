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
struct Stmt;
struct Expr;

/// Operator precedences.
///
/// See https://en.cppreference.com/w/c/language/operator_precedence.
enum class Precedence {
  Min,
  Or,    // |
  Xor,   // ^
  And,   // &
  Eq,    // == !=
  Rel,   // < > <= >=
  Shift, // << >>
  Add,   // + -
  Mul,   // * / %
  Max
};

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
  ArrayRef<Stmt *> stmts;
};

/// Base class for all statements.
struct Stmt {
  enum class Kind {
#define AST_STMT(NAME) NAME,
#include "tin/AST.def"
  };
  const Kind kind;
  Location loc;
};

/// An empty statement.
struct EmptyStmt : public Stmt {
  static bool classof(const Stmt *stmt) { return stmt->kind == Kind::Empty; }
};

/// An expression statement.
struct ExprStmt : public Stmt {
  static bool classof(const Stmt *stmt) { return stmt->kind == Kind::Expr; }
  Expr *expr;
};

/// All unary operators.
enum class UnaryOp {
#define AST_UNARY(NAME, TOKEN) NAME,
#include "tin/AST.def"
};

/// All binary operators.
enum class BinaryOp {
#define AST_BINARY(NAME, TOKEN, PREC) NAME,
#include "tin/AST.def"
};

/// Return the precedence of the given binary operator.
Precedence getPrecedence(BinaryOp op);

/// Base class for all expressions.
struct Expr {
  enum class Kind {
#define AST_EXPR(NAME) NAME,
#include "tin/AST.def"
  };
  const Kind kind;
  Location loc;
};

/// A number literal expression.
struct NumLitExpr : public Expr {
  static bool classof(const Expr *expr) { return expr->kind == Kind::NumLit; }
  APInt value;
};

/// A parenthesized expression.
struct ParenExpr : public Expr {
  static bool classof(const Expr *expr) { return expr->kind == Kind::Paren; }
  Expr *expr;
};

/// A unary expression.
struct UnaryExpr : public Expr {
  static bool classof(const Expr *expr) { return expr->kind == Kind::Unary; }
  UnaryOp op;
  Expr *arg;
};

/// A binary expression.
struct BinaryExpr : public Expr {
  static bool classof(const Expr *expr) { return expr->kind == Kind::Binary; }
  BinaryOp op;
  Expr *lhs;
  Expr *rhs;
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
    return *new (allocator) C(std::forward<C>(node));
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
