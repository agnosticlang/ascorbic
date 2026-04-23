// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once
#include "ascorbic/rule.hpp"

namespace ascorbic::rules {

class MissingReturn final : public Rule {
public:
    std::string_view id() const noexcept override { return "missing-return"; }
    std::string_view description() const noexcept override;
    void check(const Program& program, DiagnosticEngine& diag) override;

private:
    static bool always_returns(const std::vector<StmtPtr>& body);
};

} // namespace ascorbic::rules
