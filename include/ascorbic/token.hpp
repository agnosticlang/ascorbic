// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once
#include <cstdint>
#include <string>
#include <variant>
#include "ascorbic/source_location.hpp"

namespace ascorbic {

enum class TokenKind : std::uint8_t {
    Package, Import, Use, Func, Fn, Var, Let,
    If, Else, For, While, Loop, Return, Asm, Pub,

    Plus, Minus, Star, Slash, Percent,
    EqEq, BangEq, Lt, LtEq, Gt, GtEq,
    AmpAmp, PipePipe, Bang,
    Amp, Pipe, Caret, LtLt, GtGt,
    Eq, Arrow, PlusPlus,

    LParen, RParen, LBrace, RBrace, LBracket, RBracket,
    Comma, Semicolon, Colon, ColonColon, Dot, Dollar, Hash,
    DollarLParen,

    NumberLit,
    StringLit,
    Identifier,

    Eof,
    Invalid,
};

using TokenPayload = std::variant<std::monostate, std::int64_t, std::string>;

struct Token {
    TokenKind    kind;
    SourceRange  range;
    TokenPayload payload;

    std::string_view identifier() const;
    std::int64_t     number() const;
    std::string_view string_value() const;
    bool             is_keyword() const noexcept;
};

} // namespace ascorbic
