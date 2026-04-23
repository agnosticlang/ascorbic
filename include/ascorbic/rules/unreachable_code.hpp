// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once
#include "ascorbic/rule.hpp"

namespace ascorbic::rules {

class UnreachableCode final : public Rule {
public:
    std::string_view id() const noexcept override { return "unreachable-code"; }
    std::string_view description() const noexcept override;
    void check(const Program& program, DiagnosticEngine& diag) override;

private:
    static bool is_terminating(const StmtNode& stmt);
    static void check_block(const std::vector<StmtPtr>& body, DiagnosticEngine& diag);
};

} // namespace ascorbic::rules
