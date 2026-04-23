// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once
#include "ascorbic/rule.hpp"

namespace ascorbic::rules {

class EmptyBlock final : public Rule {
public:
    std::string_view id() const noexcept override { return "empty-block"; }
    std::string_view description() const noexcept override;
    void check(const Program& program, DiagnosticEngine& diag) override;

private:
    static void check_function(const Function& fn, DiagnosticEngine& diag);
    static void check_stmts(const std::vector<StmtPtr>& body, DiagnosticEngine& diag);
};

} // namespace ascorbic::rules
