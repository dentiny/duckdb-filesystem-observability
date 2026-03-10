#pragma once

// Thread safety annotations for static analysis
// These are hints for thread safety analysis tools (like clang's thread safety analysis)

#if defined(__clang__) && defined(__has_attribute)
#if __has_attribute(guarded_by)
#define DUCKDB_GUARDED_BY(x) __attribute__((guarded_by(x)))
#else
#define DUCKDB_GUARDED_BY(x)
#endif
#else
#define DUCKDB_GUARDED_BY(x)
#endif
