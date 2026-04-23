// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ascorbic/lexer.hpp"
#include <cassert>
#include <cctype>

namespace ascorbic {

SourceFile::SourceFile(std::string p, std::string t)
    : path(std::move(p)), text(std::move(t))
{
    line_starts.push_back(0);
    for (std::uint32_t i = 0; i < text.size(); ++i) {
        if (text[i] == '\n')
            line_starts.push_back(i + 1);
    }
}

std::string_view SourceFile::line_text(std::uint32_t line) const noexcept {
    if (line == 0 || line > line_starts.size()) return {};
    std::uint32_t start = line_starts[line - 1];
    std::uint32_t end;
    if (line < line_starts.size())
        end = line_starts[line] > 0 ? line_starts[line] - 1 : line_starts[line];
    else
        end = static_cast<std::uint32_t>(text.size());
    if (end < start) return {};
    return std::string_view(text).substr(start, end - start);
}

Lexer::Lexer(const SourceFile& source) noexcept
    : source_(source), pos_(0), line_(1), col_(1)
{}

char Lexer::peek(std::uint32_t ahead) const noexcept {
    std::uint32_t idx = pos_ + ahead;
    if (idx >= source_.text.size()) return '\0';
    return source_.text[idx];
}

char Lexer::advance() noexcept {
    char c = source_.text[pos_++];
    if (c == '\n') { ++line_; col_ = 1; }
    else           { ++col_; }
    return c;
}

bool Lexer::match(char expected) noexcept {
    if (pos_ >= source_.text.size()) return false;
    if (source_.text[pos_] != expected) return false;
    advance();
    return true;
}

SourceLocation Lexer::current_location() const noexcept {
    return {line_, col_, pos_};
}

void Lexer::skip_whitespace_and_comments() {
    while (pos_ < source_.text.size()) {
        char c = peek();
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            advance();
        } else if (c == '/' && peek(1) == '/') {
            while (pos_ < source_.text.size() && peek() != '\n')
                advance();
        } else if (c == '#') {
            while (pos_ < source_.text.size() && peek() != '\n')
                advance();
        } else {
            break;
        }
    }
}

Token Lexer::make_token(TokenKind kind, SourceLocation begin) const noexcept {
    return Token{kind, {begin, current_location()}, std::monostate{}};
}

Token Lexer::make_token(TokenKind kind, SourceLocation begin, TokenPayload payload) const {
    return Token{kind, {begin, current_location()}, std::move(payload)};
}

Token Lexer::error_token(std::string message, SourceLocation begin) const {
    return Token{TokenKind::Invalid, {begin, current_location()}, std::move(message)};
}

Token Lexer::lex_number(SourceLocation begin) {
    std::int64_t value = 0;
    while (pos_ < source_.text.size() && std::isdigit(static_cast<unsigned char>(peek()))) {
        value = value * 10 + (peek() - '0');
        advance();
    }
    return make_token(TokenKind::NumberLit, begin, value);
}

Token Lexer::lex_string(SourceLocation begin) {
    std::string value;
    while (pos_ < source_.text.size()) {
        char c = peek();
        if (c == '"') {
            advance();
            break;
        }
        if (c == '\\') {
            advance();
            char esc = advance();
            switch (esc) {
                case 'n':  value += '\n'; break;
                case 't':  value += '\t'; break;
                case 'r':  value += '\r'; break;
                case '"':  value += '"';  break;
                case '\\': value += '\\'; break;
                default:   value += esc;  break;
            }
            continue;
        }
        if (c == '$' && peek(1) == '(') {
            // Template expression: scan to matching )
            advance(); // $
            advance(); // (
            value += "$(";
            int depth = 1;
            while (pos_ < source_.text.size() && depth > 0) {
                char tc = advance();
                if (tc == '(') ++depth;
                else if (tc == ')') --depth;
                if (depth > 0) value += tc;
            }
            value += ')';
            continue;
        }
        value += c;
        advance();
    }
    return make_token(TokenKind::StringLit, begin, std::move(value));
}

Token Lexer::lex_identifier_or_keyword(SourceLocation begin) {
    while (pos_ < source_.text.size()) {
        char c = peek();
        if (std::isalnum(static_cast<unsigned char>(c)) || c == '_') advance();
        else break;
    }
    std::string text = source_.text.substr(begin.offset, pos_ - begin.offset);
    TokenKind kind = keyword_kind(text);
    if (kind == TokenKind::Identifier)
        return make_token(TokenKind::Identifier, begin, std::move(text));
    return make_token(kind, begin);
}

TokenKind Lexer::keyword_kind(std::string_view text) noexcept {
    if (text == "package") return TokenKind::Package;
    if (text == "import")  return TokenKind::Import;
    if (text == "use")     return TokenKind::Use;
    if (text == "func")    return TokenKind::Func;
    if (text == "fn")      return TokenKind::Fn;
    if (text == "var")     return TokenKind::Var;
    if (text == "let")     return TokenKind::Let;
    if (text == "if")      return TokenKind::If;
    if (text == "else")    return TokenKind::Else;
    if (text == "for")     return TokenKind::For;
    if (text == "while")   return TokenKind::While;
    if (text == "loop")    return TokenKind::Loop;
    if (text == "return")  return TokenKind::Return;
    if (text == "asm")     return TokenKind::Asm;
    if (text == "pub")     return TokenKind::Pub;
    return TokenKind::Identifier;
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    while (true) {
        skip_whitespace_and_comments();
        if (pos_ >= source_.text.size()) {
            tokens.push_back(make_token(TokenKind::Eof, current_location()));
            break;
        }
        SourceLocation begin = current_location();
        char c = advance();
        switch (c) {
            case '(': tokens.push_back(make_token(TokenKind::LParen,    begin)); break;
            case ')': tokens.push_back(make_token(TokenKind::RParen,    begin)); break;
            case '{': tokens.push_back(make_token(TokenKind::LBrace,    begin)); break;
            case '}': tokens.push_back(make_token(TokenKind::RBrace,    begin)); break;
            case '[': tokens.push_back(make_token(TokenKind::LBracket,  begin)); break;
            case ']': tokens.push_back(make_token(TokenKind::RBracket,  begin)); break;
            case ',': tokens.push_back(make_token(TokenKind::Comma,     begin)); break;
            case ';': tokens.push_back(make_token(TokenKind::Semicolon, begin)); break;
            case '.': tokens.push_back(make_token(TokenKind::Dot,       begin)); break;
            case '$': tokens.push_back(make_token(TokenKind::Dollar,    begin)); break;
            case '%': tokens.push_back(make_token(TokenKind::Percent,   begin)); break;
            case '^': tokens.push_back(make_token(TokenKind::Caret,     begin)); break;
            case '~': tokens.push_back(make_token(TokenKind::Caret,     begin)); break;
            case '+':
                if (match('+')) tokens.push_back(make_token(TokenKind::PlusPlus, begin));
                else            tokens.push_back(make_token(TokenKind::Plus,     begin));
                break;
            case '-':
                if (match('>')) tokens.push_back(make_token(TokenKind::Arrow,  begin));
                else            tokens.push_back(make_token(TokenKind::Minus,  begin));
                break;
            case '*': tokens.push_back(make_token(TokenKind::Star,    begin)); break;
            case '/': tokens.push_back(make_token(TokenKind::Slash,   begin)); break;
            case '!':
                if (match('=')) tokens.push_back(make_token(TokenKind::BangEq, begin));
                else            tokens.push_back(make_token(TokenKind::Bang,   begin));
                break;
            case '=':
                if (match('=')) tokens.push_back(make_token(TokenKind::EqEq, begin));
                else            tokens.push_back(make_token(TokenKind::Eq,   begin));
                break;
            case '<':
                if (match('<'))      tokens.push_back(make_token(TokenKind::LtLt, begin));
                else if (match('=')) tokens.push_back(make_token(TokenKind::LtEq, begin));
                else                 tokens.push_back(make_token(TokenKind::Lt,   begin));
                break;
            case '>':
                if (match('>'))      tokens.push_back(make_token(TokenKind::GtGt, begin));
                else if (match('=')) tokens.push_back(make_token(TokenKind::GtEq, begin));
                else                 tokens.push_back(make_token(TokenKind::Gt,   begin));
                break;
            case '&':
                if (match('&')) tokens.push_back(make_token(TokenKind::AmpAmp, begin));
                else            tokens.push_back(make_token(TokenKind::Amp,    begin));
                break;
            case '|':
                if (match('|')) tokens.push_back(make_token(TokenKind::PipePipe, begin));
                else            tokens.push_back(make_token(TokenKind::Pipe,     begin));
                break;
            case ':':
                if (match(':')) tokens.push_back(make_token(TokenKind::ColonColon, begin));
                else            tokens.push_back(make_token(TokenKind::Colon,      begin));
                break;
            case '"':
                tokens.push_back(lex_string(begin));
                break;
            default:
                if (std::isdigit(static_cast<unsigned char>(c))) {
                    // Re-parse: we consumed one digit via advance(), back up
                    --pos_; --col_;
                    tokens.push_back(lex_number(begin));
                } else if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
                    --pos_; --col_;
                    tokens.push_back(lex_identifier_or_keyword(begin));
                } else {
                    tokens.push_back(error_token(
                        std::string("unexpected character '") + c + "'", begin));
                }
                break;
        }
    }
    return tokens;
}

std::string_view Token::identifier() const {
    return std::get<std::string>(payload);
}

std::int64_t Token::number() const {
    return std::get<std::int64_t>(payload);
}

std::string_view Token::string_value() const {
    return std::get<std::string>(payload);
}

bool Token::is_keyword() const noexcept {
    return kind >= TokenKind::Package && kind <= TokenKind::Pub;
}

} // namespace ascorbic
