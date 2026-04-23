# Contributing

## Building

```sh
meson setup build
ninja -C build
```

## Code style

- C++23, no external dependencies beyond the standard library.
- All source files must include SPDX copyright and license headers.
- No decorative comment separators (`//---`, `/* === */`, etc.).
- Split logic into separate files — one concern per file.

## Adding a lint rule

1. Create `include/ascorbic/rules/your_rule.hpp` declaring a class that inherits from `Rule`.
2. Implement it in `src/rules/your_rule.cpp`.
3. Add the `.cpp` to `ascorbic_lib_sources` in `src/meson.build`.
4. Register the rule in `Linter::add_default_rules()` in `src/linter.cpp`.

Each rule receives the fully parsed `Program` AST and emits diagnostics via `DiagnosticEngine::warn()`.

## Submitting changes

Open a pull request against `main` on [GitHub](https://github.com/agnosticlang/ascorbic). Describe what the change does and, if it adds a rule, include a minimal `.agn` example that triggers it.
