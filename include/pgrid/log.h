enum pgrid_log_level {
	PGRID_SILENT = 0,
	PGRID_CRITICAL = 1,
	PGRID_ERROR = 2,
	PGRID_WARNING = 3,
	PGRID_INFO = 4,
	PGRID_DEBUG = 5,
};

extern const size_t pgrid_log_levels;

void pgrid_log_init(enum pgrid_log_level level);

void _pgrid_log(enum pgrid_log_level level, const char *fmt, ...);

#define pgrid_log(level, fmt, ...) \
	_pgrid_log(level, "[%s:%d] " fmt, __FILE__, __LINE__, ##__VA_ARGS__)
