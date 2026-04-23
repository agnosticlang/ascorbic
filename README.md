# ascorbic

A static linter for the [Agnostic](https://github.com/agnosticlang/agnostic) programming language, written in C++23.

## Building

Requirements: C++23 compiler, [Meson](https://mesonbuild.com/) 1.1+, Ninja.

```sh
meson setup build
ninja -C build
```

The binary is placed at `build/src/ascorbic`.

To install system-wide:

```sh
ninja -C build install
```

## Usage

```
ascorbic [OPTIONS] FILE...
```

Options:

| Flag | Description |
|------|-------------|
| `--disable RULE` | Disable a lint rule (repeatable) |
| `--enable RULE` | Run only the specified rule(s) (repeatable) |
| `--Werror` | Treat all warnings as errors |
| `--no-color` | Disable ANSI color output |
| `--no-caret` | Disable source line and caret display |
| `--list-rules` | Print all available rules and exit |
| `--version` | Print version and exit |
| `--help` | Print usage and exit |

Exit codes: `0` — no findings, `1` — warnings or errors, `2` — internal error.

## Rules

| Rule | Description |
|------|-------------|
| `unused-var` | Variable is declared but never read |
| `unused-import` | Import is never referenced in the file |
| `unreachable-code` | Statement is unreachable after a return |
| `missing-return` | Function with a return type does not always return a value |
| `empty-block` | Block is empty |

## Example

```
$ ascorbic main.agn

main.agn:6:9: warning[unused-var]: variable 'x' is declared but never read
    var x = 42
            ^
main.agn:3:8: warning[unused-import]: import 'stdlib/math' is never used
    import "stdlib/math"
           ^
```

## License

GNU General Public License v3.0 — see [LICENSE](LICENSE).
