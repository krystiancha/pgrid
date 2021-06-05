#include <stdarg.h>
#include <stdio.h>

#include "pgrid/log.h"

static const char *log_level_headers[] = {
	[PGRID_CRITICAL] = "[CRITICAL]",
	[PGRID_ERROR] = "[ERROR]",
	[PGRID_WARNING] = "[WARNING]",
	[PGRID_INFO] = "[INFO]",
	[PGRID_DEBUG] = "[DEBUG]",
};

const size_t pgrid_log_levels = sizeof(log_level_headers) / sizeof(char *);

static enum pgrid_log_level log_level = PGRID_WARNING;

void
pgrid_log_init(enum pgrid_log_level level)
{
	log_level = level;
}

void
_pgrid_log(enum pgrid_log_level level, const char *fmt, ...)
{
	if (level > log_level) {
		return;
	}

	va_list args;
	va_start(args, fmt);
	fprintf(stderr, "%s ", log_level_headers[level]);
	vfprintf(stderr, fmt, args);
	fprintf(stderr, "\n");
	va_end(args);
}
