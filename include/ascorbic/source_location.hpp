// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace ascorbic {

struct SourceLocation {
    std::uint32_t line{1};
    std::uint32_t column{1};
    std::uint32_t offset{0};

    constexpr bool operator==(const SourceLocation&) const noexcept = default;
    constexpr auto operator<=>(const SourceLocation&) const noexcept = default;
};

struct SourceRange {
    SourceLocation begin;
    SourceLocation end;
};

struct SourceFile {
    std::string path;
    std::string text;
    std::vector<std::uint32_t> line_starts;

    SourceFile(std::string path, std::string text);

    std::string_view line_text(std::uint32_t line) const noexcept;
};

} // namespace ascorbic
