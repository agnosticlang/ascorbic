// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>
#include "ascorbic/diagnostic.hpp"
#include "ascorbic/rule.hpp"

namespace ascorbic {

struct LinterConfig {
    std::unordered_set<std::string> disabled_rules;
    std::unordered_set<std::string> enabled_rules;
    bool warnings_as_errors = false;
};

class Linter {
public:
    explicit Linter(LinterConfig config = {});

    void add_rule(std::unique_ptr<Rule> rule);
    void add_default_rules();

    std::vector<Diagnostic> lint_file(const std::string& path,
                                      RendererOptions render_opts = RendererOptions{});

    std::size_t lint_files(const std::vector<std::string>& paths,
                           RendererOptions render_opts = RendererOptions{});

    const std::vector<std::unique_ptr<Rule>>& rules() const noexcept;

private:
    LinterConfig                       config_;
    std::vector<std::unique_ptr<Rule>> rules_;

    bool is_rule_active(const Rule& rule) const noexcept;
    static SourceFile read_source_file(const std::string& path);
};

} // namespace ascorbic
