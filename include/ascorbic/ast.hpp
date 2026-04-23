// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>
#include "ascorbic/source_location.hpp"

namespace ascorbic {

struct ExprNode;
struct StmtNode;

using ExprPtr = std::unique_ptr<ExprNode>;
using StmtPtr = std::unique_ptr<StmtNode>;

enum class BinaryOp : std::uint8_t {
    Add, Sub, Mul, Div, Mod,
    Equal, NotEqual,
    Less, LessEqual, Greater, GreaterEqual,
    And, Or,
    BitAnd, BitOr, BitXor, Shl, Shr,
    Concat,
};

enum class UnaryOp : std::uint8_t {
    Neg, Not, AddressOf, Deref,
};

struct NumberExpr   { std::int64_t value; SourceRange range; };
struct StringExpr   { std::string value;  SourceRange range; };
struct IdentExpr    { std::string name;   SourceRange range; };

struct BinaryExpr {
    BinaryOp op;
    ExprPtr  left;
    ExprPtr  right;
    SourceRange range;
};

struct UnaryExpr {
    UnaryOp op;
    ExprPtr operand;
    SourceRange range;
};

struct CallExpr {
    std::string          function;
    std::vector<ExprPtr> args;
    SourceRange          range;
};

struct ModuleCallExpr {
    std::string          module;
    std::string          function;
    std::vector<ExprPtr> args;
    bool                 double_colon;
    SourceRange          range;
};

struct ArrayAccessExpr {
    std::string name;
    ExprPtr     index;
    SourceRange range;
};

struct AddressOfExpr { ExprPtr operand; SourceRange range; };
struct DerefExpr     { ExprPtr operand; SourceRange range; };

struct ExprNode {
    std::variant<
        NumberExpr,
        StringExpr,
        IdentExpr,
        BinaryExpr,
        UnaryExpr,
        CallExpr,
        ModuleCallExpr,
        ArrayAccessExpr,
        AddressOfExpr,
        DerefExpr
    > node;

    SourceRange range() const noexcept;
};

struct Parameter {
    std::string name;
    std::string param_type;
    SourceRange range;
};

struct VarDeclStmt {
    std::string              name;
    std::optional<std::string> var_type;
    ExprPtr                  value;
    SourceRange              range;
    SourceLocation           name_loc;
};

struct ArrayDeclStmt {
    std::string name;
    std::string element_type;
    ExprPtr     size;
    SourceRange range;
    SourceLocation name_loc;
};

struct AssignStmt {
    std::string name;
    ExprPtr     value;
    SourceRange range;
};

struct ArrayAssignStmt {
    std::string name;
    ExprPtr     index;
    ExprPtr     value;
    SourceRange range;
};

struct PointerAssignStmt {
    ExprPtr     target;
    ExprPtr     value;
    SourceRange range;
};

struct IfStmt {
    ExprPtr                       condition;
    std::vector<StmtPtr>          then_body;
    std::vector<StmtPtr>          else_body;
    std::optional<SourceLocation> else_kw_loc;
    SourceRange                   range;
    SourceLocation                if_kw_loc;
};

struct ForStmt {
    ExprPtr              condition;
    std::vector<StmtPtr> body;
    SourceRange          range;
    SourceLocation       for_kw_loc;
};

struct ReturnStmt {
    ExprPtr     value;
    SourceRange range;
};

struct ExprStmt {
    ExprPtr     expression;
    SourceRange range;
};

struct InlineAsmStmt {
    std::string content;
    SourceRange range;
};

struct StmtNode {
    std::variant<
        VarDeclStmt,
        ArrayDeclStmt,
        AssignStmt,
        ArrayAssignStmt,
        PointerAssignStmt,
        IfStmt,
        ForStmt,
        ReturnStmt,
        ExprStmt,
        InlineAsmStmt
    > node;

    SourceRange range() const noexcept;
};

struct Import {
    std::string              path;
    std::optional<std::string> alias;
    SourceRange              range;

    std::string_view module_name() const noexcept;
};

struct Function {
    std::string          name;
    std::vector<Parameter> params;
    std::optional<std::string> return_type;
    std::vector<StmtPtr> body;
    bool                 is_exported;
    SourceRange          range;
    SourceLocation       name_loc;
    SourceLocation       open_brace_loc;
};

struct Program {
    std::string           package_name;
    SourceRange           package_range;
    std::vector<Import>   imports;
    std::vector<Function> functions;
};

} // namespace ascorbic
