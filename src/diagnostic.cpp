// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ascorbic/diagnostic.hpp"
#include <algorithm>
#include <cstdio>
#include <format>
#include <string>

namespace ascorbic {

DiagnosticEngine::DiagnosticEngine(std::string file_path, SinkFn sink)
    : file_path_(std::move(file_path)), sink_(std::move(sink))
{}

void DiagnosticEngine::emit(Diagnostic diag) {
    if (diag.severity == Severity::Error) ++error_count_;
    diagnostics_.push_back(diag);
    if (sink_) sink_(diag);
}

void DiagnosticEngine::error(std::string message, SourceRange range) {
    emit(Diagnostic{Severity::Error, "", std::move(message), file_path_,
                    range.begin, range});
}

void DiagnosticEngine::warn(std::string rule_id, std::string message, SourceRange range) {
    emit(Diagnostic{Severity::Warning, std::move(rule_id), std::move(message),
                    file_path_, range.begin, range});
}

void DiagnosticEngine::note(std::string message, SourceRange range) {
    emit(Diagnostic{Severity::Note, "", std::move(message), file_path_,
                    range.begin, range});
}

bool DiagnosticEngine::has_errors() const noexcept {
    return error_count_ > 0;
}

std::size_t DiagnosticEngine::count(Severity s) const noexcept {
    return static_cast<std::size_t>(
        std::count_if(diagnostics_.begin(), diagnostics_.end(),
                      [s](const Diagnostic& d) { return d.severity == s; }));
}

const std::vector<Diagnostic>& DiagnosticEngine::all() const noexcept {
    return diagnostics_;
}

DiagnosticRenderer::DiagnosticRenderer(const SourceFile& source, RendererOptions opts)
    : source_(source), opts_(opts)
{}

std::string_view DiagnosticRenderer::severity_label(Severity s) const noexcept {
    switch (s) {
        case Severity::Error:   return "error";
        case Severity::Warning: return "warning";
        case Severity::Note:    return "note";
    }
    return "unknown";
}

const char* DiagnosticRenderer::severity_color(Severity s) const noexcept {
    if (!opts_.use_color) return "";
    switch (s) {
        case Severity::Error:   return "\033[1;31m";
        case Severity::Warning: return "\033[1;33m";
        case Severity::Note:    return "\033[1;36m";
    }
    return "";
}

void DiagnosticRenderer::render(const Diagnostic& diag) const {
    const char* reset  = opts_.use_color ? "\033[0m"  : "";
    const char* bold   = opts_.use_color ? "\033[1m"  : "";
    const char* color  = severity_color(diag.severity);

    std::string label = std::string(severity_label(diag.severity));
    if (!diag.rule_id.empty())
        label += std::format("[{}]", diag.rule_id);

    std::fprintf(stderr, "%s%s:%u:%u:%s %s%s%s: %s%s%s\n",
        bold,
        diag.file_path.c_str(),
        diag.location.line,
        diag.location.column,
        reset,
        color,
        label.c_str(),
        reset,
        bold,
        diag.message.c_str(),
        reset);

    if (!opts_.show_source) return;

    auto line_text = source_.line_text(diag.location.line);
    if (line_text.empty()) return;

    std::fprintf(stderr, "    %.*s\n", static_cast<int>(line_text.size()), line_text.data());

    if (!opts_.show_caret) return;

    std::uint32_t col = diag.location.column > 0 ? diag.location.column - 1 : 0;
    std::string caret_line(4 + col, ' ');
    caret_line += '^';
    std::uint32_t width = diag.range.end.column > diag.range.begin.column
        ? diag.range.end.column - diag.range.begin.column
        : 0;
    if (width > 1 && diag.range.begin.line == diag.range.end.line)
        caret_line += std::string(width - 1, '~');
    caret_line += '\n';
    std::fprintf(stderr, "%s%s%s", color, caret_line.c_str(), reset);
}

} // namespace ascorbic
