// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ascorbic/rules/unused_import.hpp"
#include <format>

namespace ascorbic::rules {

std::string_view UnusedImport::description() const noexcept {
    return "import is never referenced in the file";
}

void UnusedImport::check(const Program& program, DiagnosticEngine& diag) {
    std::unordered_set<std::string> refs;
    for (const auto& fn : program.functions)
        collect_module_refs(fn.body, refs);

    for (const auto& imp : program.imports) {
        std::string name(imp.module_name());
        if (!refs.count(name)) {
            diag.warn("unused-import",
                std::format("import '{}' is never used", imp.path),
                imp.range);
        }
    }
}

void UnusedImport::collect_module_refs(const std::vector<StmtPtr>& body,
                                        std::unordered_set<std::string>& out)
{
    for (const auto& s : body) {
        std::visit([&](const auto& node) {
            using T = std::decay_t<decltype(node)>;
            if constexpr (std::is_same_v<T, VarDeclStmt>) {
                if (node.value) collect_from_expr(*node.value, out);
            } else if constexpr (std::is_same_v<T, ArrayDeclStmt>) {
                if (node.size) collect_from_expr(*node.size, out);
            } else if constexpr (std::is_same_v<T, AssignStmt>) {
                if (node.value) collect_from_expr(*node.value, out);
            } else if constexpr (std::is_same_v<T, ArrayAssignStmt>) {
                if (node.index) collect_from_expr(*node.index, out);
                if (node.value) collect_from_expr(*node.value, out);
            } else if constexpr (std::is_same_v<T, PointerAssignStmt>) {
                if (node.target) collect_from_expr(*node.target, out);
                if (node.value)  collect_from_expr(*node.value, out);
            } else if constexpr (std::is_same_v<T, IfStmt>) {
                if (node.condition) collect_from_expr(*node.condition, out);
                collect_module_refs(node.then_body, out);
                collect_module_refs(node.else_body, out);
            } else if constexpr (std::is_same_v<T, ForStmt>) {
                if (node.condition) collect_from_expr(*node.condition, out);
                collect_module_refs(node.body, out);
            } else if constexpr (std::is_same_v<T, ReturnStmt>) {
                if (node.value) collect_from_expr(*node.value, out);
            } else if constexpr (std::is_same_v<T, ExprStmt>) {
                if (node.expression) collect_from_expr(*node.expression, out);
            }
        }, s->node);
    }
}

void UnusedImport::collect_from_expr(const ExprNode& expr,
                                      std::unordered_set<std::string>& out)
{
    std::visit([&](const auto& node) {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, ModuleCallExpr>) {
            out.insert(node.module);
            for (const auto& a : node.args)
                if (a) collect_from_expr(*a, out);
        } else if constexpr (std::is_same_v<T, BinaryExpr>) {
            if (node.left)  collect_from_expr(*node.left, out);
            if (node.right) collect_from_expr(*node.right, out);
        } else if constexpr (std::is_same_v<T, UnaryExpr>) {
            if (node.operand) collect_from_expr(*node.operand, out);
        } else if constexpr (std::is_same_v<T, CallExpr>) {
            for (const auto& a : node.args)
                if (a) collect_from_expr(*a, out);
        } else if constexpr (std::is_same_v<T, ArrayAccessExpr>) {
            if (node.index) collect_from_expr(*node.index, out);
        } else if constexpr (std::is_same_v<T, AddressOfExpr>) {
            if (node.operand) collect_from_expr(*node.operand, out);
        } else if constexpr (std::is_same_v<T, DerefExpr>) {
            if (node.operand) collect_from_expr(*node.operand, out);
        }
    }, expr.node);
}

} // namespace ascorbic::rules
