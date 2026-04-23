// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ascorbic/rules/unreachable_code.hpp"

namespace ascorbic::rules {

std::string_view UnreachableCode::description() const noexcept {
    return "statement is unreachable after a return";
}

static bool block_always_returns(const std::vector<StmtPtr>& body) {
    for (const auto& s : body) {
        bool term = std::visit([](const auto& node) -> bool {
            using T = std::decay_t<decltype(node)>;
            if constexpr (std::is_same_v<T, ReturnStmt>) {
                return true;
            } else if constexpr (std::is_same_v<T, IfStmt>) {
                if (!node.else_kw_loc.has_value() || node.else_body.empty())
                    return false;
                return block_always_returns(node.then_body)
                    && block_always_returns(node.else_body);
            }
            return false;
        }, s->node);
        if (term) return true;
    }
    return false;
}

bool UnreachableCode::is_terminating(const StmtNode& stmt) {
    return std::visit([](const auto& node) -> bool {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, ReturnStmt>) {
            return true;
        } else if constexpr (std::is_same_v<T, IfStmt>) {
            if (!node.else_kw_loc.has_value() || node.else_body.empty())
                return false;
            return block_always_returns(node.then_body)
                && block_always_returns(node.else_body);
        }
        return false;
    }, stmt.node);
}

void UnreachableCode::check(const Program& program, DiagnosticEngine& diag) {
    for (const auto& fn : program.functions)
        check_block(fn.body, diag);
}

void UnreachableCode::check_block(const std::vector<StmtPtr>& body,
                                   DiagnosticEngine& diag)
{
    bool terminated = false;
    bool reported   = false;
    for (const auto& s : body) {
        if (terminated && !reported) {
            diag.warn("unreachable-code",
                "statement is unreachable",
                s->range());
            reported = true;
        }

        std::visit([&](const auto& node) {
            using T = std::decay_t<decltype(node)>;
            if constexpr (std::is_same_v<T, ReturnStmt>) {
                terminated = true;
            } else if constexpr (std::is_same_v<T, IfStmt>) {
                check_block(node.then_body, diag);
                check_block(node.else_body, diag);
                if (node.else_kw_loc.has_value() && !node.else_body.empty()) {
                    if (block_always_returns(node.then_body)
                        && block_always_returns(node.else_body))
                        terminated = true;
                }
            } else if constexpr (std::is_same_v<T, ForStmt>) {
                check_block(node.body, diag);
            }
        }, s->node);
    }
}

} // namespace ascorbic::rules
