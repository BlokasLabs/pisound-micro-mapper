// SPDX-License-Identifier: GPL-2.0-only
//
// pisound-micro-mapper - a daemon to facilitate Pisound Micro control mappings.
//  Copyright (C) 2023-2025  Vilniaus Blokas UAB, https://blokas.io/
//
// This file is part of pisound-micro-mapper.
//
// pisound-micro-mapper is free software: you can redistribute it and/or modify it under the terms of the
// GNU General Public License as published by the Free Software Foundation, version 2.0 of the License.
//
// pisound-micro-mapper is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even
// the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
// for more details.
//
// You should have received a copy of the GNU General Public License along with pisound-micro-mapper.
// If not, see <https://www.gnu.org/licenses/>.

#ifndef PISOUND_MICRO_LOGGER_H
#define PISOUND_MICRO_LOGGER_H

#include <iosfwd>
#include <cstdarg>

#ifndef NO_LOGGING
#	define LOG_ERROR(fmt, ...) do { if (Logger::isEnabled()) Logger::error(fmt "\n", ## __VA_ARGS__); } while (0)
#	define LOG_INFO( fmt, ...) do { if (Logger::isEnabled()) Logger::info (fmt "\n", ## __VA_ARGS__); } while (0)
#	define LOG_DEBUG(fmt, ...) do { if (Logger::isEnabled()) Logger::debug(fmt "\n", ## __VA_ARGS__); } while (0)
#else
#	define LOG_ERROR(fmt, ...) do {} while(0)
#	define LOG_INFO( fmt, ...) do {} while(0)
#	define LOG_DEBUG(fmt, ...) do {} while(0)
#endif

class ILogger;

class Logger
{
public:
	enum Level
	{
		LEVEL_ERROR,
		LEVEL_INFO,
		LEVEL_DEBUG
	};

	static bool isEnabled();

	static void setEnabled(bool on);

	static void registerLogger(ILogger &logger);

	static void logv(Level lvl, const char *format, va_list ap);

	inline static void log(Level lvl, const char *format, ...) { va_list ap; va_start(ap, format); logv(lvl, format, ap); va_end(ap); }
	inline static void error(const char *format, ...) { va_list ap; va_start(ap, format); logv(LEVEL_ERROR, format, ap); va_end(ap); }
	inline static void info (const char *format, ...) { va_list ap; va_start(ap, format); logv(LEVEL_INFO,  format, ap); va_end(ap); }
	inline static void debug(const char *format, ...) { va_list ap; va_start(ap, format); logv(LEVEL_DEBUG, format, ap); va_end(ap); }
};

class ILogger
{
public:
	virtual ~ILogger() = 0;

	virtual void log(Logger::Level lvl, const char *format, va_list ap) = 0;
};

class StdioLogger : public ILogger
{
public:
	StdioLogger(FILE *error = NULL, FILE *info = NULL, FILE *debug = NULL);

	virtual void log(Logger::Level lvl, const char *format, va_list ap) override;

private:
	FILE *m_error;
	FILE *m_info;
	FILE *m_debug;
};

#endif // PISOUND_MICRO_LOGGER_H
