// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once
#include <initializer_list>
#include <span>
#include <vector>
#include "ascorbic/ast.hpp"
#include "ascorbic/diagnostic.hpp"
#include "ascorbic/token.hpp"

namespace ascorbic {

class Parser {
public:
    Parser(std::span<const Token> tokens,
           const SourceFile& source,
           DiagnosticEngine& diagnostics);

    Program parse();

private:
    std::span<const Token> tokens_;
    std::size_t            pos_;
    const SourceFile&      source_;
    DiagnosticEngine&      diag_;

    const Token& peek(std::size_t ahead = 0) const noexcept;
    const Token& advance();
    bool         check(TokenKind kind) const noexcept;
    bool         match(TokenKind kind);
    const Token& expect(TokenKind kind, std::string_view context);
    bool         at_end() const noexcept;

    Import   parse_import();
    Function parse_function();
    Parameter parse_parameter();

    std::vector<StmtPtr> parse_block();
    StmtPtr parse_statement();
    StmtPtr parse_var_decl();
    StmtPtr parse_if();
    StmtPtr parse_for();
    StmtPtr parse_return();
    StmtPtr parse_inline_asm();
    StmtPtr parse_assign_or_expr();

    ExprPtr parse_expr();
    ExprPtr parse_expr_bp(std::uint8_t min_bp);
    ExprPtr parse_prefix();
    ExprPtr finish_call(std::string name, SourceLocation begin);
    ExprPtr finish_module_call(std::string module, std::string function,
                               bool double_colon, SourceLocation begin);

    static std::pair<std::uint8_t, std::uint8_t> infix_bp(TokenKind kind) noexcept;
    static std::uint8_t prefix_bp(TokenKind kind) noexcept;
    static BinaryOp     token_to_binary_op(TokenKind kind) noexcept;

    void synchronize(std::initializer_list<TokenKind> stop_at);
};

} // namespace ascorbic
