#include "logger.h"

#include <cstdio>
#include <cstdarg>

#include <vector>

static std::vector<ILogger*> g_loggers;

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
	case Logger::ERROR:
		dst = m_error;
		break;
	case Logger::INFO:
		dst = m_info;
		break;
	case Logger::DEBUG:
		dst = m_debug;
		break;
	}
	vfprintf(dst, format, ap);
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
