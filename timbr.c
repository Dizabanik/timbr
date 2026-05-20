/*
   timbr.c - Implementation
*/

#include "timbr.h"
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
	timbr_config.show_time = true;
	timbr_config.show_path = true;

	timbr_config.time_format = "%H:%M:%S";
	timbr_config.use_brackets = true;
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

	if (isatty(fileno(stderr))) {
		timbr_config.use_color = true;
	} else {
		timbr_config.use_color = false;
	}

	if (getenv("NO_COLOR") != NULL) {
		timbr_config.use_color = false;
	}
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

void timbr_diagnostic(TimbrLevel level, const char *title, const char *filename,
					  const char *code_line, int line_num, int col_num, int len,
					  const char *annotation) {

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

	fprintf(f, "%s%s%s: %s%s%s\n", bold, lvl_col,
			level >= TIMBR_WARN ? level_strs[level] : "NOTE", DEF_WHITE, title,
			reset);

	int gutter_width = num_digits(line_num);
	fprintf(f, "%*s%s%s%s %s%s:%d:%d%s\n", gutter_width + 1, "", dim, DEF_BLUE,
			arrow, reset, filename, line_num, col_num, reset);

	fprintf(f, "%*s %s%s%s\n", gutter_width, "", dim, pipe, reset);

	fprintf(f, "%s%*d %s%s %s%s\n", dim, gutter_width, line_num, pipe, reset,
			reset, code_line);

	fprintf(f, "%*s %s%s%s ", gutter_width, "", dim, pipe, reset);

	for (int i = 0; i < col_num - 1; i++)
		fprintf(f, " ");

	if (color)
		fprintf(f, "%s%s", bold, lvl_col);

	fputc(timbr_config.sym_caret, f);
	for (int i = 0; i < len - 1; i++)
		fputc(timbr_config.sym_underline, f);

	if (annotation) {
		fprintf(f, " %s", annotation);
	}
	fprintf(f, "%s\n", reset);

	fprintf(f, "%*s %s%s%s\n", gutter_width, "", dim, pipe, reset);
}
