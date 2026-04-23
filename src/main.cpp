// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ascorbic/diagnostic.hpp"
#include "ascorbic/linter.hpp"
#include <cstdio>
#include <string>
#include <unistd.h>
#include <vector>

static void print_usage(const char* argv0) {
    std::fprintf(stderr,
        "Usage: %s [OPTIONS] FILE...\n"
        "\n"
        "Options:\n"
        "  --disable RULE   Disable a lint rule (repeatable)\n"
        "  --enable RULE    Run only the specified rule(s) (repeatable)\n"
        "  --Werror         Treat warnings as errors\n"
        "  --no-color       Disable ANSI color output\n"
        "  --no-caret       Disable source line and caret display\n"
        "  --list-rules     Print available rules and exit\n"
        "  --version        Print version and exit\n"
        "  --help           Print this help and exit\n",
        argv0);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 2;
    }

    ascorbic::LinterConfig config;
    ascorbic::RendererOptions render_opts;
    render_opts.use_color = isatty(STDERR_FILENO) != 0;

    std::vector<std::string> files;
    bool list_rules = false;

    for (int i = 1; i < argc; ++i) {
        std::string_view arg(argv[i]);
        if (arg == "--help") {
            print_usage(argv[0]);
            return 0;
        }
        if (arg == "--version") {
            std::fprintf(stdout, "ascorbic 0.1.0\n");
            return 0;
        }
        if (arg == "--list-rules") {
            list_rules = true;
        } else if (arg == "--Werror") {
            config.warnings_as_errors = true;
        } else if (arg == "--no-color") {
            render_opts.use_color = false;
        } else if (arg == "--no-caret") {
            render_opts.show_caret = false;
        } else if (arg == "--disable" && i + 1 < argc) {
            config.disabled_rules.insert(argv[++i]);
        } else if (arg == "--enable" && i + 1 < argc) {
            config.enabled_rules.insert(argv[++i]);
        } else if (arg.starts_with("--")) {
            std::fprintf(stderr, "ascorbic: unknown option '%s'\n", argv[i]);
            return 2;
        } else {
            files.emplace_back(argv[i]);
        }
    }

    ascorbic::Linter linter(std::move(config));
    linter.add_default_rules();

    if (list_rules) {
        for (const auto& rule : linter.rules()) {
            std::fprintf(stdout, "  %-20s  %s\n",
                std::string(rule->id()).c_str(),
                std::string(rule->description()).c_str());
        }
        return 0;
    }

    if (files.empty()) {
        std::fprintf(stderr, "ascorbic: no input files\n");
        return 2;
    }

    std::size_t warning_count = 0;
    std::size_t error_count   = 0;

    for (const auto& path : files) {
        try {
            auto diags = linter.lint_file(path, render_opts);
            for (const auto& d : diags) {
                if (d.severity == ascorbic::Severity::Error)        ++error_count;
                else if (d.severity == ascorbic::Severity::Warning) ++warning_count;
            }
        } catch (const std::exception& e) {
            std::fprintf(stderr, "ascorbic: %s\n", e.what());
            ++error_count;
        }
    }

    if (error_count > 0) return 1;
    if (warning_count > 0) return 1;
    return 0;
}
