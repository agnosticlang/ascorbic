// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once
#include <vector>
#include "ascorbic/token.hpp"

namespace ascorbic {

class Lexer {
public:
    explicit Lexer(const SourceFile& source) noexcept;

    std::vector<Token> tokenize();

private:
    const SourceFile& source_;
    std::uint32_t     pos_;
    std::uint32_t     line_;
    std::uint32_t     col_;

    char           peek(std::uint32_t ahead = 0) const noexcept;
    char           advance() noexcept;
    bool           match(char expected) noexcept;
    void           skip_whitespace_and_comments();
    SourceLocation current_location() const noexcept;

    Token lex_number(SourceLocation begin);
    Token lex_string(SourceLocation begin);
    Token lex_identifier_or_keyword(SourceLocation begin);
    Token make_token(TokenKind kind, SourceLocation begin) const noexcept;
    Token make_token(TokenKind kind, SourceLocation begin, TokenPayload payload) const;
    Token error_token(std::string message, SourceLocation begin) const;

    static TokenKind keyword_kind(std::string_view text) noexcept;
};

} // namespace ascorbic
