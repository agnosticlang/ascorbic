// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ascorbic/rules/missing_return.hpp"
#include <format>

namespace ascorbic::rules {

std::string_view MissingReturn::description() const noexcept {
    return "function with a return type does not always return a value";
}

bool MissingReturn::always_returns(const std::vector<StmtPtr>& body) {
    for (const auto& s : body) {
        bool term = std::visit([](const auto& node) -> bool {
            using T = std::decay_t<decltype(node)>;
            if constexpr (std::is_same_v<T, ReturnStmt>) {
                return true;
            } else if constexpr (std::is_same_v<T, IfStmt>) {
                if (!node.else_kw_loc.has_value() || node.else_body.empty())
                    return false;
                return MissingReturn::always_returns(node.then_body)
                    && MissingReturn::always_returns(node.else_body);
            }
            return false;
        }, s->node);
        if (term) return true;
    }
    return false;
}

void MissingReturn::check(const Program& program, DiagnosticEngine& diag) {
    for (const auto& fn : program.functions) {
        if (!fn.return_type.has_value()) continue;
        if (*fn.return_type == "void") continue;
        if (always_returns(fn.body)) continue;

        diag.warn("missing-return",
            std::format("function '{}' with return type '{}' does not always return a value",
                fn.name, *fn.return_type),
            {fn.open_brace_loc, fn.range.end});
    }
}

} // namespace ascorbic::rules
