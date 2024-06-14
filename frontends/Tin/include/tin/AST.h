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
#include "llvm/ADT/TypeSwitch.h"

namespace circt {
namespace tin {
namespace ast {

struct Item;
struct Stmt;
struct Expr;
struct Type;

// AST nodes that can be referred to by name.
struct ModPort;
struct LetStmt;
using Binding = PointerUnion<ModPort *, LetStmt *>;

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

  template <typename V, typename... Args>
  void walk(V &visitor, Args &&...args) {
    for (auto *item : items)
      visitor.visit(*item, std::forward<Args>(args)...);
  }
};

//===----------------------------------------------------------------------===//
// Items
//===----------------------------------------------------------------------===//

/// Base class for all items.
struct Item {
  enum class Kind {
#define AST_ITEM(NAME) NAME,
#include "tin/AST.def"
  };
  const Kind kind;
  Location loc;

  template <typename V, typename... Args>
  void walk(V &&visitor, Args &&...args) {}
};

/// A module port.
struct ModPort {
  Location loc;
  bool isOutput;
  StringAttr name;
  Type *type;

  template <typename V, typename... Args>
  void walk(V &&visitor, Args &&...args) {
    visitor.visit(*type, std::forward<Args>(args)...);
  }
};

/// A module definition.
struct ModItem : public Item {
  static bool classof(const Item *item) { return item->kind == Kind::Mod; }
  StringAttr name;
  ArrayRef<ModPort *> ports;
  ArrayRef<Stmt *> stmts;

  template <typename V, typename... Args>
  void walk(V &visitor, Args &&...args) {
    for (auto *port : ports)
      visitor.visit(*port, std::forward<Args>(args)...);
    for (auto *stmt : stmts)
      visitor.visit(*stmt, std::forward<Args>(args)...);
  }
};

//===----------------------------------------------------------------------===//
// Statements
//===----------------------------------------------------------------------===//

/// Base class for all statements.
struct Stmt {
  enum class Kind {
#define AST_STMT(NAME) NAME,
#include "tin/AST.def"
  };
  const Kind kind;
  Location loc;

  template <typename V, typename... Args>
  void walk(V &&visitor, Args &&...args) {}
};

/// An empty statement.
struct EmptyStmt : public Stmt {
  static bool classof(const Stmt *stmt) { return stmt->kind == Kind::Empty; }
};

/// An expression statement.
struct ExprStmt : public Stmt {
  static bool classof(const Stmt *stmt) { return stmt->kind == Kind::Expr; }
  Expr *expr;

  template <typename V, typename... Args>
  void walk(V &visitor, Args &&...args) {
    visitor.visit(*expr, std::forward<Args>(args)...);
  }
};

/// An output assignment statement.
struct OutStmt : public Stmt {
  static bool classof(const Stmt *stmt) { return stmt->kind == Kind::Out; }
  StringAttr name;
  Expr *value;
  Binding binding = nullptr;

  template <typename V, typename... Args>
  void walk(V &visitor, Args &&...args) {
    visitor.visit(*value, std::forward<Args>(args)...);
  }
};

/// A let statement.
struct LetStmt : public Stmt {
  static bool classof(const Stmt *stmt) { return stmt->kind == Kind::Let; }
  StringAttr name;
  Type *type;
  Expr *value;

  template <typename V, typename... Args>
  void walk(V &visitor, Args &&...args) {
    visitor.visit(*type, std::forward<Args>(args)...);
    visitor.visit(*value, std::forward<Args>(args)...);
  }
};

//===----------------------------------------------------------------------===//
// Expressions
//===----------------------------------------------------------------------===//

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

  template <typename V, typename... Args>
  void walk(V &&visitor, Args &&...args) {}
};

/// A number literal expression.
struct NumLitExpr : public Expr {
  static bool classof(const Expr *expr) { return expr->kind == Kind::NumLit; }
  APInt value;
};

/// An identifier expression.
struct IdentExpr : public Expr {
  static bool classof(const Expr *expr) { return expr->kind == Kind::Ident; }
  StringAttr name;
  Binding binding = nullptr;
};

/// A parenthesized expression.
struct ParenExpr : public Expr {
  static bool classof(const Expr *expr) { return expr->kind == Kind::Paren; }
  Expr *expr;

  template <typename V, typename... Args>
  void walk(V &visitor, Args &&...args) {
    visitor.visit(*expr, std::forward<Args>(args)...);
  }
};

/// A unary expression.
struct UnaryExpr : public Expr {
  static bool classof(const Expr *expr) { return expr->kind == Kind::Unary; }
  UnaryOp op;
  Expr *arg;

  template <typename V, typename... Args>
  void walk(V &visitor, Args &&...args) {
    visitor.visit(*arg, std::forward<Args>(args)...);
  }
};

/// A binary expression.
struct BinaryExpr : public Expr {
  static bool classof(const Expr *expr) { return expr->kind == Kind::Binary; }
  BinaryOp op;
  Expr *lhs;
  Expr *rhs;

  template <typename V, typename... Args>
  void walk(V &visitor, Args &&...args) {
    visitor.visit(*lhs, std::forward<Args>(args)...);
    visitor.visit(*rhs, std::forward<Args>(args)...);
  }
};

//===----------------------------------------------------------------------===//
// Types
//===----------------------------------------------------------------------===//

/// Base class for all types.
struct Type {
  enum class Kind {
#define AST_TYPE(NAME) NAME,
#include "tin/AST.def"
  };
  const Kind kind;
  Location loc;

  template <typename V, typename... Args>
  void walk(V &&visitor, Args &&...args) {}
};

/// A signless integer type.
struct IntType : public Type {
  static bool classof(const Type *type) { return type->kind == Kind::Int; }
  unsigned width;
};

//===----------------------------------------------------------------------===//
// Visitor
//===----------------------------------------------------------------------===//

template <typename Derived>
struct Visitor {
  /// Visit an AST node. Override this in subclasses.
  template <class T, typename... Args>
  decltype(auto) visit(T &node, Args &&...args) {
    return static_cast<Derived *>(this)->visitDefault(
        node, std::forward<Args>(args)...);
  }

  /// Default visitation behavior of visiting all children of `node`.
  template <class T, typename... Args>
  decltype(auto) visitDefault(T &node, Args &&...args) {
    // return node.walk(*static_cast<Derived *>(this), args...);
  }

  /// Dispatch to concrete items.
  template <typename... Args>
  decltype(auto) visit(Item &item, Args &&...args) {
    return TypeSwitch<Item *>(&item)
#define AST_ITEM(NAME)                                                         \
  .template Case<NAME##Item>([&](auto *item) {                                 \
    return static_cast<Derived *>(this)->visit(*item,                          \
                                               std::forward<Args>(args)...);   \
  })
#include "tin/AST.def"
        ;
  }
  /// Dispatch to concrete statements.
  template <typename... Args>
  decltype(auto) visit(Stmt &stmt, Args &&...args) {
    return TypeSwitch<Stmt *>(&stmt)
#define AST_STMT(NAME)                                                         \
  .template Case<NAME##Stmt>([&](auto *stmt) {                                 \
    return static_cast<Derived *>(this)->visit(*stmt,                          \
                                               std::forward<Args>(args)...);   \
  })
#include "tin/AST.def"
        ;
  }

  /// Dispatch to concrete expressions.
  template <typename... Args>
  decltype(auto) visit(Expr &expr, Args &&...args) {
    return TypeSwitch<Expr *>(&expr)
#define AST_EXPR(NAME)                                                         \
  .template Case<NAME##Expr>([&](auto *expr) {                                 \
    return static_cast<Derived *>(this)->visit(*expr,                          \
                                               std::forward<Args>(args)...);   \
  })
#include "tin/AST.def"
        ;
  }
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
