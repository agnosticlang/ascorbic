// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ascorbic/linter.hpp"
#include "ascorbic/lexer.hpp"
#include "ascorbic/parser.hpp"
#include "ascorbic/rules/empty_block.hpp"
#include "ascorbic/rules/missing_return.hpp"
#include "ascorbic/rules/unreachable_code.hpp"
#include "ascorbic/rules/unused_import.hpp"
#include "ascorbic/rules/unused_var.hpp"
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace ascorbic {

Linter::Linter(LinterConfig config)
    : config_(std::move(config))
{}

void Linter::add_rule(std::unique_ptr<Rule> rule) {
    rules_.push_back(std::move(rule));
}

void Linter::add_default_rules() {
    add_rule(std::make_unique<rules::UnusedVar>());
    add_rule(std::make_unique<rules::UnusedImport>());
    add_rule(std::make_unique<rules::UnreachableCode>());
    add_rule(std::make_unique<rules::MissingReturn>());
    add_rule(std::make_unique<rules::EmptyBlock>());
}

const std::vector<std::unique_ptr<Rule>>& Linter::rules() const noexcept {
    return rules_;
}

bool Linter::is_rule_active(const Rule& rule) const noexcept {
    if (!config_.disabled_rules.empty()
        && config_.disabled_rules.count(std::string(rule.id())))
        return false;
    if (!config_.enabled_rules.empty()
        && !config_.enabled_rules.count(std::string(rule.id())))
        return false;
    return true;
}

SourceFile Linter::read_source_file(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open())
        throw std::runtime_error("cannot open file: " + path);
    std::ostringstream ss;
    ss << file.rdbuf();
    return SourceFile(path, ss.str());
}

std::vector<Diagnostic> Linter::lint_file(const std::string& path,
                                           RendererOptions render_opts)
{
    SourceFile source = read_source_file(path);
    DiagnosticRenderer renderer(source, render_opts);

    DiagnosticEngine diag(path,
        [&renderer](const Diagnostic& d) { renderer.render(d); });

    Lexer lexer(source);
    auto tokens = lexer.tokenize();

    for (const auto& t : tokens) {
        if (t.kind == TokenKind::Invalid)
            diag.error(std::string(t.string_value()), t.range);
    }

    Parser parser(tokens, source, diag);
    Program program = parser.parse();

    if (!diag.has_errors()) {
        for (const auto& rule : rules_) {
            if (is_rule_active(*rule))
                rule->check(program, diag);
        }
    }

    return diag.all();
}

std::size_t Linter::lint_files(const std::vector<std::string>& paths,
                                RendererOptions render_opts)
{
    std::size_t total = 0;
    for (const auto& path : paths) {
        try {
            auto diags = lint_file(path, render_opts);
            total += diags.size();
        } catch (const std::exception& e) {
            std::fprintf(stderr, "ascorbic: error: %s\n", e.what());
            ++total;
        }
    }
    return total;
}

} // namespace ascorbic
