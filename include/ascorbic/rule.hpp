// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once
#include <string_view>
#include "ascorbic/ast.hpp"
#include "ascorbic/diagnostic.hpp"

namespace ascorbic {

class Rule {
public:
    virtual ~Rule() = default;

    virtual std::string_view id() const noexcept = 0;
    virtual std::string_view description() const noexcept = 0;
    virtual Severity default_severity() const noexcept { return Severity::Warning; }

    virtual void check(const Program& program, DiagnosticEngine& diag) = 0;

    Rule(const Rule&)            = delete;
    Rule& operator=(const Rule&) = delete;

protected:
    Rule() = default;
};

} // namespace ascorbic
