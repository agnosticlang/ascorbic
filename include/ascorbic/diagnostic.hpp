// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once
#include <cstdint>
#include <functional>
#include <string>
#include <vector>
#include "ascorbic/source_location.hpp"

namespace ascorbic {

enum class Severity : std::uint8_t {
    Error,
    Warning,
    Note,
};

struct Diagnostic {
    Severity       severity;
    std::string    rule_id;
    std::string    message;
    std::string    file_path;
    SourceLocation location;
    SourceRange    range;
};

class DiagnosticEngine {
public:
    using SinkFn = std::function<void(const Diagnostic&)>;

    explicit DiagnosticEngine(std::string file_path, SinkFn sink = nullptr);

    void emit(Diagnostic diag);
    void error(std::string message, SourceRange range);
    void warn(std::string rule_id, std::string message, SourceRange range);
    void note(std::string message, SourceRange range);

    bool        has_errors() const noexcept;
    std::size_t count(Severity s) const noexcept;
    const std::vector<Diagnostic>& all() const noexcept;

private:
    std::string              file_path_;
    SinkFn                   sink_;
    std::vector<Diagnostic>  diagnostics_;
    std::size_t              error_count_{0};
};

struct RendererOptions {
    bool use_color   = true;
    bool show_source = true;
    bool show_caret  = true;
};

class DiagnosticRenderer {
public:
    DiagnosticRenderer(const SourceFile& source, RendererOptions opts = RendererOptions{});

    void render(const Diagnostic& diag) const;

private:
    const SourceFile& source_;
    RendererOptions   opts_;

    std::string_view severity_label(Severity s) const noexcept;
    const char*      severity_color(Severity s) const noexcept;
};

} // namespace ascorbic
