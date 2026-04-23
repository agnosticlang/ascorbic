// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ascorbic/rules/unused_var.hpp"
#include <format>
#include <unordered_map>

namespace ascorbic::rules {

std::string_view UnusedVar::description() const noexcept {
    return "variable is declared but never read";
}

void UnusedVar::check(const Program& program, DiagnosticEngine& diag) {
    for (const auto& fn : program.functions)
        check_function(fn, diag);
}

void UnusedVar::check_function(const Function& fn, DiagnosticEngine& diag) {
    struct VarInfo {
        SourceRange range;
        SourceLocation name_loc;
        bool used = false;
    };
    std::unordered_map<std::string, VarInfo> vars;

    // Collect declarations
    std::function<void(const std::vector<StmtPtr>&)> collect_decls;
    collect_decls = [&](const std::vector<StmtPtr>& body) {
        for (const auto& s : body) {
            std::visit([&](const auto& node) {
                using T = std::decay_t<decltype(node)>;
                if constexpr (std::is_same_v<T, VarDeclStmt>) {
                    if (!vars.count(node.name))
                        vars[node.name] = VarInfo{node.range, node.name_loc, false};
                } else if constexpr (std::is_same_v<T, ArrayDeclStmt>) {
                    if (!vars.count(node.name))
                        vars[node.name] = VarInfo{node.range, node.name_loc, false};
                } else if constexpr (std::is_same_v<T, IfStmt>) {
                    collect_decls(node.then_body);
                    collect_decls(node.else_body);
                } else if constexpr (std::is_same_v<T, ForStmt>) {
                    collect_decls(node.body);
                }
            }, s->node);
        }
    };
    collect_decls(fn.body);

    // Collect reads
    std::unordered_set<std::string> reads;
    collect_reads(fn.body, reads);

    for (const auto& [name, info] : vars) {
        if (!reads.count(name)) {
            diag.warn("unused-var",
                std::format("variable '{}' is declared but never read", name),
                {info.name_loc, info.name_loc});
        }
    }
}

void UnusedVar::collect_reads(const std::vector<StmtPtr>& body,
                               std::unordered_set<std::string>& out)
{
    for (const auto& s : body) {
        std::visit([&](const auto& node) {
            using T = std::decay_t<decltype(node)>;
            if constexpr (std::is_same_v<T, VarDeclStmt>) {
                if (node.value) collect_reads_expr(*node.value, out);
            } else if constexpr (std::is_same_v<T, ArrayDeclStmt>) {
                if (node.size) collect_reads_expr(*node.size, out);
            } else if constexpr (std::is_same_v<T, AssignStmt>) {
                // LHS name is a write, not a read; only RHS counts
                if (node.value) collect_reads_expr(*node.value, out);
            } else if constexpr (std::is_same_v<T, ArrayAssignStmt>) {
                if (node.index) collect_reads_expr(*node.index, out);
                if (node.value) collect_reads_expr(*node.value, out);
            } else if constexpr (std::is_same_v<T, PointerAssignStmt>) {
                if (node.target) collect_reads_expr(*node.target, out);
                if (node.value)  collect_reads_expr(*node.value, out);
            } else if constexpr (std::is_same_v<T, IfStmt>) {
                if (node.condition) collect_reads_expr(*node.condition, out);
                collect_reads(node.then_body, out);
                collect_reads(node.else_body, out);
            } else if constexpr (std::is_same_v<T, ForStmt>) {
                if (node.condition) collect_reads_expr(*node.condition, out);
                collect_reads(node.body, out);
            } else if constexpr (std::is_same_v<T, ReturnStmt>) {
                if (node.value) collect_reads_expr(*node.value, out);
            } else if constexpr (std::is_same_v<T, ExprStmt>) {
                if (node.expression) collect_reads_expr(*node.expression, out);
            }
        }, s->node);
    }
}

void UnusedVar::collect_reads_expr(const ExprNode& expr,
                                    std::unordered_set<std::string>& out)
{
    std::visit([&](const auto& node) {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, IdentExpr>) {
            out.insert(node.name);
        } else if constexpr (std::is_same_v<T, BinaryExpr>) {
            if (node.left)  collect_reads_expr(*node.left, out);
            if (node.right) collect_reads_expr(*node.right, out);
        } else if constexpr (std::is_same_v<T, UnaryExpr>) {
            if (node.operand) collect_reads_expr(*node.operand, out);
        } else if constexpr (std::is_same_v<T, CallExpr>) {
            for (const auto& a : node.args)
                if (a) collect_reads_expr(*a, out);
        } else if constexpr (std::is_same_v<T, ModuleCallExpr>) {
            for (const auto& a : node.args)
                if (a) collect_reads_expr(*a, out);
        } else if constexpr (std::is_same_v<T, ArrayAccessExpr>) {
            out.insert(node.name);
            if (node.index) collect_reads_expr(*node.index, out);
        } else if constexpr (std::is_same_v<T, AddressOfExpr>) {
            if (node.operand) collect_reads_expr(*node.operand, out);
        } else if constexpr (std::is_same_v<T, DerefExpr>) {
            if (node.operand) collect_reads_expr(*node.operand, out);
        }
    }, expr.node);
}

} // namespace ascorbic::rules
