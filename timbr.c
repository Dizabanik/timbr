/*
   timbr.c - Implementation
*/

#include "timbr.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define C_RESET "\033[0m"
#define C_BOLD "\033[1m"
#define C_DIM "\033[2m"

#define DEF_GRAY "\033[90m"
#define DEF_RED "\033[31m"
#define DEF_GREEN "\033[32m"
#define DEF_YELLOW "\033[33m"
#define DEF_BLUE "\033[34m"
#define DEF_MAGENTA "\033[35m"
#define DEF_CYAN "\033[36m"
#define DEF_WHITE "\033[37m"

TimbrConfig timbr_config = {0};

static const char *level_strs[] = {"TRACE", "DEBUG", " INFO",
								   " WARN", "ERROR", "FATAL"};

// Counters for the end-of-compilation summary ("N warnings emitted").
int timbr_error_count = 0;
int timbr_warning_count = 0;

// --color=always|never|auto override, set by the driver before timbr_init.
// 0 = auto (default), 1 = force on, -1 = force off.
int timbr_color_override = 0;

// rustc-style lowercase severity words for diagnostic headers ("error",
// "warning"); level_strs above stays fixed-width for the log format.
static const char *diag_level_word(TimbrLevel level) {
	switch (level) {
	case TIMBR_WARN:
		return "warning";
	case TIMBR_ERROR:
		return "error";
	case TIMBR_FATAL:
		return "fatal error";
	default:
		return "note";
	}
}

static const char *get_level_color(TimbrLevel level) {
	switch (level) {
	case TIMBR_TRACE:
		return timbr_config.col_trace;
	case TIMBR_DEBUG:
		return timbr_config.col_debug;
	case TIMBR_INFO:
		return timbr_config.col_info;
	case TIMBR_WARN:
		return timbr_config.col_warn;
	case TIMBR_ERROR:
		return timbr_config.col_error;
	case TIMBR_FATAL:
		return timbr_config.col_fatal;
	default:
		return C_RESET;
	}
}

static void set_defaults(void) {
	timbr_config.output_stream = stderr;
	timbr_config.show_time = false;
	timbr_config.show_path = true;

	timbr_config.time_format = "%H:%M:%S";
	timbr_config.use_brackets = false;
	timbr_config.bold_levels = true;

	timbr_config.sym_arrow = "-->";
	timbr_config.sym_pipe = "|";
	timbr_config.sym_underline = '~';
	timbr_config.sym_caret = '^';

	timbr_config.col_trace = DEF_GRAY;
	timbr_config.col_debug = DEF_BLUE;
	timbr_config.col_info = DEF_GREEN;
	timbr_config.col_warn = DEF_YELLOW;
	timbr_config.col_error = DEF_RED;
	timbr_config.col_fatal = DEF_MAGENTA;

	bool tty = isatty(fileno(stderr)) != 0;
	if (timbr_color_override == 1)
		tty = true;
	else if (timbr_color_override == -1)
		tty = false;
	if (tty && getenv("NO_COLOR") == NULL)
		timbr_config.use_color = true;
	else
		timbr_config.use_color = false;
}

void timbr_init(void) { set_defaults(); }

void timbr_log(TimbrLevel level, const char *file, int line, const char *fmt,
			   ...) {
	if (timbr_config.output_stream == NULL)
		timbr_init();

	FILE *f = timbr_config.output_stream;
	bool color = timbr_config.use_color;
	const char *lvl_col = get_level_color(level);

	if (timbr_config.show_time) {
		time_t t = time(NULL);
		struct tm *tm_info = localtime(&t);
		char time_buf[64];
		strftime(time_buf, sizeof(time_buf), timbr_config.time_format, tm_info);

		if (color)
			fprintf(f, "%s%s ", DEF_GRAY, time_buf);
		else
			fprintf(f, "%s ", time_buf);
	}

	if (color) {
		fprintf(f, "%s%s", timbr_config.bold_levels ? C_BOLD : "", lvl_col);
	}

	if (timbr_config.use_brackets)
		fprintf(f, "[");

	fprintf(f, "%s", level_strs[level]);
	if (timbr_config.use_brackets)
		fprintf(f, "]");

	if (color)
		fprintf(f, "%s ", C_RESET);
	else
		fprintf(f, " ");

	if (timbr_config.show_path && file) {
		if (color)
			fprintf(f, "%s%s:%d%s \t", DEF_GRAY, file, line, C_RESET);
		else
			fprintf(f, "%s:%d \t", file, line);
	}

	va_list args;
	va_start(args, fmt);
	if (color && level >= TIMBR_ERROR)
		fprintf(f, "%s", C_BOLD);
	vfprintf(f, fmt, args);
	if (color)
		fprintf(f, "%s", C_RESET);
	va_end(args);

	fprintf(f, "\n");
}

static int num_digits(int n) {
	if (n == 0)
		return 1;
	int count = 0;
	while (n != 0) {
		n /= 10;
		count++;
	}
	return count;
}

// Visible width of a UTF-8 string: codepoints, not bytes. Tabs expand to
// their visual width so carets stay aligned under the right column.
static int utf8_width(const char *s, int nbytes) {
	int w = 0;
	for (int i = 0; i < nbytes && s[i]; i++) {
		unsigned char c = (unsigned char)s[i];
		// Skip UTF-8 continuation bytes; each lead byte is one cell. This
		// miscounts combining/wide chars, which is acceptable for source
		// snippets -- alignment matters more than typography.
		if ((c & 0xC0) == 0x80)
			continue;
		w++;
	}
	return w;
}

// Render one diagnostic in rustc style. All spans must be on the same line
// as spans[0] (the primary); that covers everything kawac emits today and
// keeps the renderer simple enough to stay correct.
void timbr_diagnostic_ex(TimbrLevel level, const char *code,
						 const char *title, const TimbrSpan *spans,
						 int nspans, const char *const *notes,
						 const char *const *helps) {
	if (timbr_config.output_stream == NULL)
		timbr_init();
	FILE *f = timbr_config.output_stream;
	bool color = timbr_config.use_color;

	const char *lvl_col = color ? get_level_color(level) : "";
	const char *reset = color ? C_RESET : "";
	const char *bold = color ? C_BOLD : "";
	const char *dim = color ? C_DIM : "";
	const char *arrow = timbr_config.sym_arrow;
	const char *pipe = timbr_config.sym_pipe;

	const TimbrSpan *primary = &spans[0];
	int line_num = primary->line_num;

	if (level == TIMBR_WARN)
		timbr_warning_count++;
	if (level >= TIMBR_ERROR)
		timbr_error_count++;

	// Header: error[E0001]: title / warning: title
	fprintf(f, "%s%s%s", bold, lvl_col,
			level >= TIMBR_WARN ? diag_level_word(level) : "note");
	if (code && level >= TIMBR_ERROR)
		fprintf(f, "[%s]", code);
	fprintf(f, "%s: %s\n", reset, title);

	// Location line.
	int gutter = num_digits(line_num);
	const char *arrow_col = color ? DEF_BLUE : "";
	fprintf(f, "%*s%s%s%s %s%s:%d:%d%s\n", gutter + 2, "", dim, arrow_col,
			arrow, reset, primary->filename, line_num, primary->col_num,
			reset);

	fprintf(f, "%*s %s%s%s\n", gutter, "", dim, pipe, reset);

	// Source line with tabs expanded to 4 spaces for caret alignment.
	const char *src_line = primary->code_line ? primary->code_line : "";
	int line_bytes = (int)strlen(src_line);
	int tab_pad_total = 0;
	fprintf(f, "%s%*d %s%s ", dim, gutter, line_num, pipe, reset);
	{
		int col_so_far = 0;
		for (int i = 0; i < line_bytes; i++) {
			if (src_line[i] == '\t') {
				int pad = 4 - (col_so_far % 4);
				for (int t = 0; t < pad; t++)
					fputc(' ', f);
				col_so_far += pad;
				tab_pad_total += pad;
			} else {
				fputc(src_line[i], f);
				col_so_far++;
			}
		}
	}
	fprintf(f, "%s\n", reset);

	// Marker line: spaces up to the first span's column, then labels.
	fprintf(f, "%*s %s%s%s ", gutter, "", dim, pipe, reset);
	int col = 1;
	for (int s = 0; s < nspans; s++) {
		const TimbrSpan *sp = &spans[s];
		// Advance to this span's column (tab-aware).
		while (col < sp->col_num) {
			int pad =
				(col > 0 && src_line[col - 1] == '\t') ? 1 : 1;
			(void)pad;
			fputc(' ', f);
			col++;
		}
		int span_w = sp->len > 0 ? sp->len : 1;
		span_w = utf8_width(src_line + (sp->col_num - 1), span_w);
		if (span_w < 1)
			span_w = 1;
		if (color)
			fprintf(f, "%s%s", bold, lvl_col);
		if (sp->is_secondary) {
			fputc(timbr_config.sym_underline, f);
			for (int i = 1; i < span_w; i++)
				fputc(timbr_config.sym_underline, f);
		} else {
			fputc('^', f);
			for (int i = 1; i < span_w; i++)
				fputc('^', f);
		}
		if (color)
			fprintf(f, "%s", reset);
		col += span_w;
		// The FIRST label rides the marker line; later same-line labels are
		// rare in kawac and render after two spaces.
		if (sp->label) {
			if (color)
				fprintf(f, "%s%s%s", dim, sp->is_secondary ? "" : "",
						reset);
			fprintf(f, " %s", sp->label);
		}
	}
	fprintf(f, "%s\n", reset);

	fprintf(f, "%*s %s%s%s\n", gutter, "", dim, pipe, reset);

	// Trailing help/notes, rustc style: `help:` inline or `= note:` block.
	const char *cyan = color ? DEF_CYAN : "";
	const char *gray = color ? DEF_GRAY : "";
	for (int h = 0; helps && helps[h]; h++) {
		fprintf(f, "%*s %s%s%s %shelp%s: %s%s\n", gutter, "", dim, pipe,
				reset, cyan, reset, helps[h], reset);
	}
	for (int nn = 0; notes && notes[nn]; nn++) {
		fprintf(f, "%*s %s%s%s %s= note%s: %s%s\n", gutter, "", dim, pipe,
				reset, gray, reset, notes[nn], reset);
	}
}

void timbr_diagnostic(TimbrLevel level, const char *title, const char *filename,
					  const char *code_line, int line_num, int col_num, int len,
					  const char *annotation) {
	TimbrSpan span = {.code_line = code_line,
					  .filename = filename,
					  .line_num = line_num,
					  .col_num = col_num,
					  .len = len,
					  .label = annotation,
					  .is_secondary = 0};
	timbr_diagnostic_ex(level, NULL, title, &span, 1, NULL, NULL);
}
