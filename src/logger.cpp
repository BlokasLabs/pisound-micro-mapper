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

#include "logger.h"

#include <cstdio>
#include <cstdarg>
#include <algorithm>

#include <vector>

static std::vector<ILogger*> g_loggers;
static Logger::Level g_level = Logger::LEVEL_NONE;

bool Logger::isEnabled()
{
	return g_level != LEVEL_NONE;
}

bool Logger::canLog(Level lvl)
{
	return lvl != LEVEL_NONE && g_level >= lvl;
}

Logger::Level Logger::getLevel()
{
	return g_level;
}

bool Logger::tryParseLevel(int value, Level &lvl)
{
	switch (value)
	{
	case 0:
		lvl = LEVEL_NONE;
		return true;
	case 1:
		lvl = LEVEL_ERROR;
		return true;
	case 2:
		lvl = LEVEL_INFO;
		return true;
	case 3:
		lvl = LEVEL_DEBUG;
		return true;
	default:
		return false;
	}
}

void Logger::setEnabled(bool on)
{
	g_level = on ? LEVEL_DEBUG : LEVEL_NONE;
}

void Logger::setLevel(Level lvl)
{
	g_level = std::max(LEVEL_NONE, std::min(lvl, LEVEL_DEBUG));
}

void Logger::registerLogger(ILogger &logger)
{
	g_loggers.push_back(&logger);
}

void Logger::logv(Level lvl, const char *format, va_list ap)
{
	if (!canLog(lvl))
		return;

	for (auto l : g_loggers)
	{
		va_list copy;
		va_copy(copy, ap);
		l->log(lvl, format, copy);
		va_end(copy);
	}
}

StdioLogger::StdioLogger(FILE *error, FILE *info, FILE *debug)
	:m_error(error ? error : stderr)
	,m_info(info ? info : stdout)
	,m_debug(debug ? debug : stderr)
{
}

void StdioLogger::log(Logger::Level lvl, const char *format, va_list ap)
{
	FILE *dst;
	switch (lvl)
	{
	default:
		fprintf(m_error, "Unknown log level %d! Logging as error!\n", lvl);
		// fallthrough intentional.
	case Logger::LEVEL_ERROR:
		dst = m_error;
		break;
	case Logger::LEVEL_INFO:
		dst = m_info;
		break;
	case Logger::LEVEL_DEBUG:
		dst = m_debug;
		break;
	}
	vfprintf(dst, format, ap);
}
