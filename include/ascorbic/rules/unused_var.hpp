// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once
#include <unordered_set>
#include "ascorbic/rule.hpp"

namespace ascorbic::rules {

class UnusedVar final : public Rule {
public:
    std::string_view id() const noexcept override { return "unused-var"; }
    std::string_view description() const noexcept override;
    void check(const Program& program, DiagnosticEngine& diag) override;

private:
    static void check_function(const Function& fn, DiagnosticEngine& diag);
    static void collect_reads(const std::vector<StmtPtr>& body,
                              std::unordered_set<std::string>& out);
    static void collect_reads_expr(const ExprNode& expr,
                                   std::unordered_set<std::string>& out);
};

} // namespace ascorbic::rules
