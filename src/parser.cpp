// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ascorbic/parser.hpp"
#include <cassert>
#include <format>

namespace ascorbic {

SourceRange ExprNode::range() const noexcept {
    return std::visit([](const auto& n) { return n.range; }, node);
}

SourceRange StmtNode::range() const noexcept {
    return std::visit([](const auto& n) { return n.range; }, node);
}

std::string_view Import::module_name() const noexcept {
    if (alias) return *alias;
    auto pos = path.rfind('/');
    if (pos == std::string::npos) return path;
    return std::string_view(path).substr(pos + 1);
}

Parser::Parser(std::span<const Token> tokens,
               const SourceFile& source,
               DiagnosticEngine& diagnostics)
    : tokens_(tokens), pos_(0), source_(source), diag_(diagnostics)
{}

const Token& Parser::peek(std::size_t ahead) const noexcept {
    std::size_t idx = pos_ + ahead;
    if (idx >= tokens_.size()) return tokens_.back(); // Eof
    return tokens_[idx];
}

const Token& Parser::advance() {
    const Token& t = tokens_[pos_];
    if (pos_ + 1 < tokens_.size()) ++pos_;
    return t;
}

bool Parser::check(TokenKind kind) const noexcept {
    return peek().kind == kind;
}

bool Parser::match(TokenKind kind) {
    if (!check(kind)) return false;
    advance();
    return true;
}

const Token& Parser::expect(TokenKind kind, std::string_view context) {
    if (check(kind)) return advance();
    diag_.error(
        std::format("expected {} in {}", static_cast<int>(kind), context),
        peek().range);
    return peek();
}

bool Parser::at_end() const noexcept {
    return check(TokenKind::Eof);
}

void Parser::synchronize(std::initializer_list<TokenKind> stop_at) {
    while (!at_end()) {
        for (TokenKind k : stop_at)
            if (check(k)) return;
        advance();
    }
}

Program Parser::parse() {
    Program program;
    if (match(TokenKind::Package)) {
        SourceLocation pkg_begin = peek().range.begin;
        if (check(TokenKind::Identifier)) {
            program.package_name = std::string(advance().identifier());
            program.package_range = {pkg_begin, peek().range.begin};
        } else {
            diag_.error("expected package name", peek().range);
        }
    }
    while (!at_end()) {
        if (check(TokenKind::Import) || check(TokenKind::Use)) {
            advance();
            program.imports.push_back(parse_import());
        } else if (check(TokenKind::Pub) || check(TokenKind::Func) || check(TokenKind::Fn)) {
            program.functions.push_back(parse_function());
        } else {
            diag_.error("expected import or function definition", peek().range);
            synchronize({TokenKind::Func, TokenKind::Fn, TokenKind::Pub, TokenKind::Import});
        }
    }
    return program;
}

Import Parser::parse_import() {
    Import imp;
    imp.range.begin = peek().range.begin;
    if (check(TokenKind::StringLit)) {
        imp.path = std::string(advance().string_value());
    } else {
        diag_.error("expected import path string", peek().range);
        imp.path = "";
    }
    imp.range.end = peek().range.begin;
    return imp;
}

Parameter Parser::parse_parameter() {
    Parameter param;
    param.range.begin = peek().range.begin;
    if (check(TokenKind::Identifier)) {
        param.name = std::string(advance().identifier());
    } else {
        diag_.error("expected parameter name", peek().range);
    }
    if (match(TokenKind::Colon)) {
        if (check(TokenKind::Identifier)) {
            param.param_type = std::string(advance().identifier());
        } else {
            diag_.error("expected parameter type", peek().range);
        }
    }
    param.range.end = peek().range.begin;
    return param;
}

Function Parser::parse_function() {
    Function fn;
    fn.range.begin = peek().range.begin;
    fn.is_exported = match(TokenKind::Pub);
    match(TokenKind::Func) || match(TokenKind::Fn);
    if (check(TokenKind::Identifier)) {
        fn.name_loc = peek().range.begin;
        fn.name = std::string(advance().identifier());
    } else {
        diag_.error("expected function name", peek().range);
        fn.name = "<error>";
        fn.name_loc = peek().range.begin;
    }
    expect(TokenKind::LParen, "function parameters");
    while (!check(TokenKind::RParen) && !at_end()) {
        fn.params.push_back(parse_parameter());
        if (!match(TokenKind::Comma)) break;
    }
    expect(TokenKind::RParen, "function parameters");
    if (match(TokenKind::Arrow)) {
        if (check(TokenKind::Identifier)) {
            fn.return_type = std::string(advance().identifier());
        } else {
            diag_.error("expected return type", peek().range);
        }
    }
    fn.open_brace_loc = peek().range.begin;
    fn.body = parse_block();
    fn.range.end = peek().range.begin;
    return fn;
}

std::vector<StmtPtr> Parser::parse_block() {
    expect(TokenKind::LBrace, "block");
    std::vector<StmtPtr> stmts;
    while (!check(TokenKind::RBrace) && !at_end()) {
        while (match(TokenKind::Semicolon)) {}
        if (check(TokenKind::RBrace)) break;
        stmts.push_back(parse_statement());
        match(TokenKind::Semicolon);
    }
    expect(TokenKind::RBrace, "block");
    return stmts;
}

StmtPtr Parser::parse_statement() {
    if (check(TokenKind::Var) || check(TokenKind::Let))
        return parse_var_decl();
    if (check(TokenKind::If))
        return parse_if();
    if (check(TokenKind::For) || check(TokenKind::While) || check(TokenKind::Loop))
        return parse_for();
    if (check(TokenKind::Return))
        return parse_return();
    if (check(TokenKind::Asm))
        return parse_inline_asm();
    return parse_assign_or_expr();
}

StmtPtr Parser::parse_var_decl() {
    SourceLocation begin = peek().range.begin;
    advance(); // var or let

    if (check(TokenKind::Identifier) && peek(1).kind == TokenKind::Colon
        && peek(2).kind == TokenKind::LBracket)
    {
        // Array declaration: var name : [size] type
        ArrayDeclStmt stmt;
        stmt.range.begin = begin;
        stmt.name_loc    = peek().range.begin;
        stmt.name        = std::string(advance().identifier());
        advance(); // :
        advance(); // [
        stmt.size = parse_expr();
        expect(TokenKind::RBracket, "array size");
        if (check(TokenKind::Identifier)) {
            stmt.element_type = std::string(advance().identifier());
        } else {
            diag_.error("expected element type", peek().range);
        }
        stmt.range.end = peek().range.begin;
        return std::make_unique<StmtNode>(StmtNode{std::move(stmt)});
    }

    VarDeclStmt stmt;
    stmt.range.begin = begin;
    if (check(TokenKind::Identifier)) {
        stmt.name_loc = peek().range.begin;
        stmt.name     = std::string(advance().identifier());
    } else {
        diag_.error("expected variable name", peek().range);
        stmt.name     = "<error>";
        stmt.name_loc = peek().range.begin;
    }
    if (match(TokenKind::Colon)) {
        if (check(TokenKind::Identifier)) {
            stmt.var_type = std::string(advance().identifier());
        } else {
            diag_.error("expected type", peek().range);
        }
    }
    if (match(TokenKind::Eq)) {
        stmt.value = parse_expr();
    }
    stmt.range.end = peek().range.begin;
    return std::make_unique<StmtNode>(StmtNode{std::move(stmt)});
}

StmtPtr Parser::parse_if() {
    IfStmt stmt;
    stmt.if_kw_loc  = peek().range.begin;
    stmt.range.begin = peek().range.begin;
    advance(); // if
    stmt.condition  = parse_expr();
    stmt.then_body  = parse_block();
    if (match(TokenKind::Else)) {
        stmt.else_kw_loc = peek().range.begin;
        stmt.else_body   = parse_block();
    }
    stmt.range.end = peek().range.begin;
    return std::make_unique<StmtNode>(StmtNode{std::move(stmt)});
}

StmtPtr Parser::parse_for() {
    ForStmt stmt;
    stmt.for_kw_loc  = peek().range.begin;
    stmt.range.begin = peek().range.begin;
    TokenKind kw = advance().kind; // for / while / loop
    if (kw != TokenKind::Loop && !check(TokenKind::LBrace)) {
        stmt.condition = parse_expr();
    }
    stmt.body      = parse_block();
    stmt.range.end = peek().range.begin;
    return std::make_unique<StmtNode>(StmtNode{std::move(stmt)});
}

StmtPtr Parser::parse_return() {
    ReturnStmt stmt;
    stmt.range.begin = peek().range.begin;
    advance(); // return
    auto is_stmt_start = [&] {
        auto k = peek().kind;
        return k == TokenKind::Semicolon || k == TokenKind::RBrace
            || k == TokenKind::Var    || k == TokenKind::Let
            || k == TokenKind::If     || k == TokenKind::Else
            || k == TokenKind::For    || k == TokenKind::While
            || k == TokenKind::Loop   || k == TokenKind::Return
            || k == TokenKind::Asm    || k == TokenKind::Pub
            || k == TokenKind::Func   || k == TokenKind::Fn
            || k == TokenKind::Eof;
    };
    if (!is_stmt_start()) {
        stmt.value = parse_expr();
    }
    stmt.range.end = peek().range.begin;
    return std::make_unique<StmtNode>(StmtNode{std::move(stmt)});
}

StmtPtr Parser::parse_inline_asm() {
    InlineAsmStmt stmt;
    stmt.range.begin = peek().range.begin;
    advance(); // asm
    if (check(TokenKind::StringLit)) {
        stmt.content = std::string(advance().string_value());
    } else if (check(TokenKind::LBrace)) {
        advance(); // {
        while (!check(TokenKind::RBrace) && !at_end()) {
            if (check(TokenKind::StringLit))
                stmt.content += std::string(advance().string_value());
            else
                advance();
        }
        match(TokenKind::RBrace);
    }
    stmt.range.end = peek().range.begin;
    return std::make_unique<StmtNode>(StmtNode{std::move(stmt)});
}

StmtPtr Parser::parse_assign_or_expr() {
    SourceLocation begin = peek().range.begin;
    auto lhs = parse_expr();

    if (!check(TokenKind::Eq)) {
        SourceRange r{begin, lhs->range().end};
        return std::make_unique<StmtNode>(StmtNode{ExprStmt{std::move(lhs), r}});
    }
    advance(); // =
    auto rhs = parse_expr();
    SourceRange r{begin, rhs->range().end};

    auto& node = lhs->node;
    if (auto* id = std::get_if<IdentExpr>(&node)) {
        return std::make_unique<StmtNode>(StmtNode{AssignStmt{id->name, std::move(rhs), r}});
    }
    if (auto* arr = std::get_if<ArrayAccessExpr>(&node)) {
        return std::make_unique<StmtNode>(StmtNode{
            ArrayAssignStmt{arr->name, std::move(arr->index), std::move(rhs), r}});
    }
    if (auto* deref = std::get_if<DerefExpr>(&node)) {
        return std::make_unique<StmtNode>(StmtNode{
            PointerAssignStmt{std::move(deref->operand), std::move(rhs), r}});
    }
    diag_.error("invalid assignment target", r);
    return std::make_unique<StmtNode>(StmtNode{ExprStmt{std::move(lhs), r}});
}

// Pratt binding powers
std::pair<std::uint8_t, std::uint8_t> Parser::infix_bp(TokenKind kind) noexcept {
    switch (kind) {
        case TokenKind::PipePipe: return {10, 11};
        case TokenKind::AmpAmp:   return {20, 21};
        case TokenKind::Pipe:     return {30, 31};
        case TokenKind::Caret:    return {40, 41};
        case TokenKind::Amp:      return {50, 51};
        case TokenKind::EqEq:
        case TokenKind::BangEq:   return {60, 61};
        case TokenKind::Lt:
        case TokenKind::LtEq:
        case TokenKind::Gt:
        case TokenKind::GtEq:     return {70, 71};
        case TokenKind::LtLt:
        case TokenKind::GtGt:     return {80, 81};
        case TokenKind::Plus:
        case TokenKind::Minus:
        case TokenKind::PlusPlus: return {90, 91};
        case TokenKind::Star:
        case TokenKind::Slash:
        case TokenKind::Percent:  return {100, 101};
        default:                  return {0, 0};
    }
}

std::uint8_t Parser::prefix_bp(TokenKind kind) noexcept {
    switch (kind) {
        case TokenKind::Minus:
        case TokenKind::Bang:
        case TokenKind::Amp:
        case TokenKind::Star:  return 110;
        default:               return 0;
    }
}

BinaryOp Parser::token_to_binary_op(TokenKind kind) noexcept {
    switch (kind) {
        case TokenKind::Plus:     return BinaryOp::Add;
        case TokenKind::Minus:    return BinaryOp::Sub;
        case TokenKind::Star:     return BinaryOp::Mul;
        case TokenKind::Slash:    return BinaryOp::Div;
        case TokenKind::Percent:  return BinaryOp::Mod;
        case TokenKind::EqEq:     return BinaryOp::Equal;
        case TokenKind::BangEq:   return BinaryOp::NotEqual;
        case TokenKind::Lt:       return BinaryOp::Less;
        case TokenKind::LtEq:     return BinaryOp::LessEqual;
        case TokenKind::Gt:       return BinaryOp::Greater;
        case TokenKind::GtEq:     return BinaryOp::GreaterEqual;
        case TokenKind::AmpAmp:   return BinaryOp::And;
        case TokenKind::PipePipe: return BinaryOp::Or;
        case TokenKind::Amp:      return BinaryOp::BitAnd;
        case TokenKind::Pipe:     return BinaryOp::BitOr;
        case TokenKind::Caret:    return BinaryOp::BitXor;
        case TokenKind::LtLt:     return BinaryOp::Shl;
        case TokenKind::GtGt:     return BinaryOp::Shr;
        case TokenKind::PlusPlus: return BinaryOp::Concat;
        default:                  return BinaryOp::Add;
    }
}

ExprPtr Parser::parse_expr() {
    return parse_expr_bp(0);
}

ExprPtr Parser::parse_prefix() {
    SourceLocation begin = peek().range.begin;
    TokenKind kind = peek().kind;
    std::uint8_t bp = prefix_bp(kind);
    if (bp > 0) {
        advance();
        auto operand = parse_expr_bp(bp);
        SourceRange r{begin, operand->range().end};
        if (kind == TokenKind::Minus)
            return std::make_unique<ExprNode>(ExprNode{UnaryExpr{UnaryOp::Neg,  std::move(operand), r}});
        if (kind == TokenKind::Bang)
            return std::make_unique<ExprNode>(ExprNode{UnaryExpr{UnaryOp::Not,  std::move(operand), r}});
        if (kind == TokenKind::Amp)
            return std::make_unique<ExprNode>(ExprNode{AddressOfExpr{std::move(operand), r}});
        if (kind == TokenKind::Star)
            return std::make_unique<ExprNode>(ExprNode{DerefExpr{std::move(operand), r}});
    }

    // Primary expressions
    if (check(TokenKind::NumberLit)) {
        auto& t = advance();
        return std::make_unique<ExprNode>(ExprNode{NumberExpr{t.number(), t.range}});
    }
    if (check(TokenKind::StringLit)) {
        auto& t = advance();
        return std::make_unique<ExprNode>(ExprNode{StringExpr{std::string(t.string_value()), t.range}});
    }
    if (check(TokenKind::LParen)) {
        advance();
        auto e = parse_expr();
        expect(TokenKind::RParen, "parenthesized expression");
        return e;
    }
    if (check(TokenKind::Identifier)) {
        SourceLocation id_begin = peek().range.begin;
        std::string name = std::string(advance().identifier());

        // module.function(args) or module::function(args)
        if (check(TokenKind::Dot) || check(TokenKind::ColonColon)) {
            bool dc = check(TokenKind::ColonColon);
            advance();
            if (check(TokenKind::Identifier)) {
                std::string fn = std::string(advance().identifier());
                if (check(TokenKind::LParen))
                    return finish_module_call(name, fn, dc, id_begin);
                // module.field access — treat as ident for linter purposes
            }
        }

        // function call
        if (check(TokenKind::LParen))
            return finish_call(name, id_begin);

        // array access
        if (check(TokenKind::LBracket)) {
            advance();
            auto idx = parse_expr();
            expect(TokenKind::RBracket, "array access");
            SourceRange r{id_begin, peek().range.begin};
            return std::make_unique<ExprNode>(ExprNode{ArrayAccessExpr{name, std::move(idx), r}});
        }

        SourceRange r{id_begin, {peek().range.begin.line, peek().range.begin.column,
                                  static_cast<std::uint32_t>(id_begin.offset + name.size())}};
        return std::make_unique<ExprNode>(ExprNode{IdentExpr{name, r}});
    }

    // Fallback: error
    SourceRange r{begin, peek().range.begin};
    diag_.error("expected expression", r);
    return std::make_unique<ExprNode>(ExprNode{IdentExpr{"<error>", r}});
}

ExprPtr Parser::finish_call(std::string name, SourceLocation begin) {
    advance(); // (
    std::vector<ExprPtr> args;
    while (!check(TokenKind::RParen) && !at_end()) {
        args.push_back(parse_expr());
        if (!match(TokenKind::Comma)) break;
    }
    expect(TokenKind::RParen, "function call");
    SourceRange r{begin, peek().range.begin};
    return std::make_unique<ExprNode>(ExprNode{CallExpr{std::move(name), std::move(args), r}});
}

ExprPtr Parser::finish_module_call(std::string module, std::string function,
                                    bool double_colon, SourceLocation begin)
{
    advance(); // (
    std::vector<ExprPtr> args;
    while (!check(TokenKind::RParen) && !at_end()) {
        args.push_back(parse_expr());
        if (!match(TokenKind::Comma)) break;
    }
    expect(TokenKind::RParen, "module call");
    SourceRange r{begin, peek().range.begin};
    return std::make_unique<ExprNode>(ExprNode{
        ModuleCallExpr{std::move(module), std::move(function),
                       std::move(args), double_colon, r}});
}

ExprPtr Parser::parse_expr_bp(std::uint8_t min_bp) {
    auto lhs = parse_prefix();

    while (true) {
        TokenKind op = peek().kind;
        auto [l_bp, r_bp] = infix_bp(op);
        if (l_bp <= min_bp) break;
        advance();
        auto rhs = parse_expr_bp(r_bp);
        SourceRange r{lhs->range().begin, rhs->range().end};
        lhs = std::make_unique<ExprNode>(ExprNode{
            BinaryExpr{token_to_binary_op(op), std::move(lhs), std::move(rhs), r}});
    }
    return lhs;
}

} // namespace ascorbic
