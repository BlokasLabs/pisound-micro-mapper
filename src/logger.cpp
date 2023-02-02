#include "logger.h"

#include <cstdio>
#include <cstdarg>

#include <vector>

static std::vector<ILogger*> g_loggers;
static bool g_enabled = false;

bool Logger::isEnabled()
{
	return g_enabled;
}

void Logger::setEnabled(bool on)
{
	g_enabled = on;
}

void Logger::registerLogger(ILogger &logger)
{
	g_loggers.push_back(&logger);
}

void Logger::logv(Level lvl, const char *format, va_list ap)
{
	for (auto l : g_loggers)
		l->log(lvl, format, ap);
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
