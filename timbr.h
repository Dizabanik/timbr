/*
   timbr.h - diagnostics for compilers: spans, labels, notes, help.

   Rendering takes the best of rustc (labels under source, error codes,
   help/note sub-diagnostics), clang (carets, fix-it phrasing), Zig
   (everything on stderr, no noise) and Go (terse messages). Output is
   UTF-8 plain text to stderr; ANSI color only when the stream is a tty
   and NO_COLOR is unset.
*/

#ifndef TIMBR_H
#define TIMBR_H

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>

// -- config --

typedef enum {
	TIMBR_TRACE = 0,
	TIMBR_DEBUG,
	TIMBR_INFO,
	TIMBR_WARN,
	TIMBR_ERROR,
	TIMBR_FATAL
} TimbrLevel;

typedef struct {
	// -- core --
	bool use_color;		 // Auto-detected by default
	bool show_time;		 // Show timestamp? Default: false for diagnostics
	bool show_path;		 // Show file path? Default: true
	FILE *output_stream; // Default: stderr

	// -- formatting --
	const char *time_format; // strftime format. Default: "%H:%M:%S"
	bool use_brackets;		 // Draw [INFO] vs INFO. Default: true
	bool bold_levels;		 // Bold the level name? Default: true

	// -- colors --
	const char *col_trace;
	const char *col_debug;
	const char *col_info;
	const char *col_warn;
	const char *col_error;
	const char *col_fatal;

	// -- Diagnostic Symbols --
	const char *sym_arrow; // Pointer to file. Default: "-->"
	const char *sym_pipe;  // Vertical bar. Default: "|"
	char sym_underline;	   // Character for underlining. Default: '~'
	char sym_caret;		   // Character for pointing. Default: '^'

} TimbrConfig;

// -- api --

// Global singleton configuration. Modify fields directly after init.
extern TimbrConfig timbr_config;

// Initialize defaults (call once at startup)
void timbr_init(void);

// --color=always|never|auto override, set by the driver BEFORE timbr_init.
// 0 = auto (default), 1 = force on, -1 = force off.
extern int timbr_color_override;

// Running diagnostic tallies, incremented by timbr_diagnostic_ex and read
// by the driver's end-of-compile summary line.
extern int timbr_error_count;
extern int timbr_warning_count;

// Standard logging function
void timbr_log(TimbrLevel level, const char *file, int line, const char *fmt,
			   ...);

// -- compiler api --

/*
   One labeled span inside a diagnostic. label may be NULL for a bare
   underline (primary span usually carries the message).
*/
typedef struct {
	const char *code_line; // full source line text (NUL-terminated)
	const char *filename;
	int line_num;
	int col_num; // 1-based
	int len;	 // span length in bytes (>=1)
	const char *label;
	int is_secondary; // secondary labels render dimmer with their own text
} TimbrSpan;

/*
   Prints a rustc-style diagnostic:

     error[E0308]: mismatched types
       --> src/main.kawa:6:13
        |
      6 |     let x = ;
        |             ^ expected type
        |
        = note: trailing punctuation starts a new statement

   level:      Severity (WARN/ERROR/etc.)
   code:       Error code like "E0308", or NULL to omit [E####]
   title:      The main message ("mismatched types")
   spans:      Array of spans; spans[0] is primary (its location is shown)
   nspans:     Number of spans
   note/help:  Optional trailing lines; rendered as `= note:` / `help:`
*/
void timbr_diagnostic_ex(TimbrLevel level, const char *code,
						 const char *title, const TimbrSpan *spans,
						 int nspans, const char *const *notes,
						 const char *const *helps);

/* Back-compat single-span entry point. */
void timbr_diagnostic(TimbrLevel level, const char *title, const char *filename,
					  const char *code_line, int line_num, int col_num, int len,
					  const char *annotation);

// -- Convenience Macros --

#define timbr_trace(...) timbr_log(TIMBR_TRACE, __FILE__, __LINE__, __VA_ARGS__)
#define timbr_debug(...) timbr_log(TIMBR_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define timbr_info(...) timbr_log(TIMBR_INFO, __FILE__, __LINE__, __VA_ARGS__)
#define timbr_warn(...) timbr_log(TIMBR_WARN, __FILE__, __LINE__, __VA_ARGS__)
#define timbr_err(...) timbr_log(TIMBR_ERROR, __FILE__, __LINE__, __VA_ARGS__)
#define timbr_fat(...) timbr_log(TIMBR_FATAL, __FILE__, __LINE__, __VA_ARGS__)

#endif // TIMBR_H
