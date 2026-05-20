# timbr.h

A minimalist logging and diagnostics library for C. Zero dependencies, designed for high-performance applications and custom compilers.

---

## Features

- Six log levels: `TRACE` -> `DEBUG` -> `INFO` -> `WARN` -> `ERROR` -> `FATAL`
- Auto-detected ANSI color output
- Timestamps, file paths, and bracketed level labels — all configurable
- **Compiler diagnostics** with inline code context, squiggly underlines, and caret annotations
- Single global config struct — no init boilerplate required beyond one call

---

## Installation

Copy `timbr.h` and `timbr.c` into your project.

```c
#include "timbr.h"
```

Call once at startup:

```c
timbr_init();
```

---

## Usage

### Logging

```c
timbr_info("Server started on port %d", port);
timbr_warn("Config value missing, using default");
timbr_err("Failed to open file: %s", path);
```

Output:

```
[12:04:31] [INFO]  src/main.c:42 — Server started on port 8080
[12:04:31] [WARN]  src/main.c:57 — Config value missing, using default
[12:04:31] [ERROR] src/main.c:63 — Failed to open file: config.toml
```

### Macros

| Macro              | Level         |
| ------------------ | ------------- |
| `timbr_trace(...)` | `TIMBR_TRACE` |
| `timbr_debug(...)` | `TIMBR_DEBUG` |
| `timbr_info(...)`  | `TIMBR_INFO`  |
| `timbr_warn(...)`  | `TIMBR_WARN`  |
| `timbr_err(...)`   | `TIMBR_ERROR` |
| `timbr_fat(...)`   | `TIMBR_FATAL` |

All macros automatically capture `__FILE__` and `__LINE__`.

### Compiler Diagnostics

```c
timbr_diagnostic(
    TIMBR_ERROR,
    "Unexpected token",
    "parser.c",
    "int x = ;",
    14, 8, 1,
    "expected expression"
);
```

Output (would be colored):

```
error: Unexpected token
 --> parser.c:14:8
  |
14|   int x = ;
  |           ^ expected expression
```

---

## Configuration

Modify `timbr_config` directly after calling `timbr_init()`.

```c
timbr_init();
timbr_config.use_color    = true;
timbr_config.show_time    = false;
timbr_config.output_stream = stdout;
timbr_config.time_format  = "%Y-%m-%d %H:%M:%S";
```

### All Options

| Field                     | Type          | Default      | Description                  |
| ------------------------- | ------------- | ------------ | ---------------------------- |
| `use_color`               | `bool`        | auto         | Enable ANSI color output     |
| `show_time`               | `bool`        | `true`       | Prepend timestamp            |
| `show_path`               | `bool`        | `true`       | Prepend file path and line   |
| `output_stream`           | `FILE*`       | `stderr`     | Output destination           |
| `time_format`             | `const char*` | `"%H:%M:%S"` | `strftime` format string     |
| `use_brackets`            | `bool`        | `true`       | `[INFO]` vs `INFO`           |
| `bold_levels`             | `bool`        | `true`       | Bold the level label         |
| `col_trace` … `col_fatal` | `const char*` | built-in     | ANSI escape codes per level  |
| `sym_arrow`               | `const char*` | `"-->"`      | Diagnostic file pointer      |
| `sym_pipe`                | `const char*` | `"\|"`       | Diagnostic gutter bar        |
| `sym_underline`           | `char`        | `'~'`        | Squiggly underline character |
| `sym_caret`               | `char`        | `'^'`        | Caret pointer character      |

---

## API Reference

```c
void timbr_init(void);

void timbr_log(TimbrLevel level, const char *file, int line,
               const char *fmt, ...);

void timbr_diagnostic(TimbrLevel level, const char *title,
                      const char *filename, const char *code_line,
                      int line_num, int col_num, int len,
                      const char *annotation);
```

---

## License

MIT
