#pragma once

#include <cstring>
#include <iostream>

#include "duckdb/common/exception.hpp"
#include "duckdb/common/string.hpp"
#include "duckdb/common/string_util.hpp"

namespace duckdb {

#ifdef _WIN32
#define SYSCALL_THROW_IF_ERROR(ret)                                                                                    \
	do {                                                                                                               \
		if ((ret) < 0) {                                                                                               \
			throw IOException("Failed to execute syscall");                                                            \
		}                                                                                                              \
	} while (0)

#define SYSCALL_EXIT_IF_ERROR(ret)                                                                                     \
	do {                                                                                                               \
		if ((ret) < 0) {                                                                                               \
			std::cerr << "Failed to execute syscall" << std::endl;                                                     \
			std::exit(-1);                                                                                             \
		}                                                                                                              \
	} while (0)
#else
#include <cerrno>
#define SYSCALL_THROW_IF_ERROR(ret)                                                                                    \
	do {                                                                                                               \
		if ((ret) < 0) {                                                                                               \
			int errnum = errno;                                                                                        \
			const string err_msg = StringUtil::Format("Failed to execute syscall with errno %d and error message %s",  \
			                                          errnum, std::strerror(errnum));                                  \
			throw IOException(err_msg);                                                                                \
		}                                                                                                              \
	} while (0)

#define SYSCALL_EXIT_IF_ERROR(ret)                                                                                     \
	do {                                                                                                               \
		if ((ret) < 0) {                                                                                               \
			int errnum = errno;                                                                                        \
			const string err_msg = StringUtil::Format("Failed to execute syscall with errno %d and error message %s",  \
			                                          errnum, std::strerror(errnum));                                  \
			std::cerr << err_msg << std::endl;                                                                         \
			std::exit(-1);                                                                                             \
		}                                                                                                              \
	} while (0)
#endif

} // namespace duckdb
