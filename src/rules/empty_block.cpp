// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ascorbic/rules/empty_block.hpp"
#include <format>

namespace ascorbic::rules {

std::string_view EmptyBlock::description() const noexcept {
    return "block is empty";
}

void EmptyBlock::check(const Program& program, DiagnosticEngine& diag) {
    for (const auto& fn : program.functions)
        check_function(fn, diag);
}

void EmptyBlock::check_function(const Function& fn, DiagnosticEngine& diag) {
    if (fn.body.empty()) {
        diag.warn("empty-block",
            std::format("function '{}' has an empty body", fn.name),
            {fn.open_brace_loc, fn.range.end});
        return;
    }
    check_stmts(fn.body, diag);
}

void EmptyBlock::check_stmts(const std::vector<StmtPtr>& body,
                               DiagnosticEngine& diag)
{
    for (const auto& s : body) {
        std::visit([&](const auto& node) {
            using T = std::decay_t<decltype(node)>;
            if constexpr (std::is_same_v<T, IfStmt>) {
                if (node.then_body.empty()) {
                    diag.warn("empty-block",
                        "if block is empty",
                        {node.if_kw_loc, node.if_kw_loc});
                } else {
                    check_stmts(node.then_body, diag);
                }
                if (node.else_kw_loc.has_value() && node.else_body.empty()) {
                    diag.warn("empty-block",
                        "else block is empty",
                        {*node.else_kw_loc, *node.else_kw_loc});
                } else {
                    check_stmts(node.else_body, diag);
                }
            } else if constexpr (std::is_same_v<T, ForStmt>) {
                if (node.body.empty()) {
                    diag.warn("empty-block",
                        "loop body is empty",
                        {node.for_kw_loc, node.for_kw_loc});
                } else {
                    check_stmts(node.body, diag);
                }
            }
        }, s->node);
    }
}

} // namespace ascorbic::rules
