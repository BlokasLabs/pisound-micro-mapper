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
