/*
   timbr.h - A minimalist, C logging & diagnostic library.
   Designed for high-performance applications and custom compilers.
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
	bool show_time;		 // Show timestamp? Default: true
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

// Standard logging function
void timbr_log(TimbrLevel level, const char *file, int line, const char *fmt,
			   ...);

// -- compiler api --

/*
	Prints a diagnostic message with code context.

	level:       Severity (WARN/ERROR/etc.)
	title:       The main error message (e.g., "Unexpected token")
	filename:    Name of the file causing error
	code_line:   The actual string of code (e.g., "int x = ;")
	line_num:    The line number
	col_num:     The column number
	len:         Length of the squiggly underline
	annotation:  Text under the squiggles (e.g., "expected expression here")
*/
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
