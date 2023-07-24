#ifndef PISOUND_MICRO_MAPPER_CONTROL_SERVER_H
#define PISOUND_MICRO_MAPPER_CONTROL_SERVER_H

#include <cstddef>
#include <cstdint>

struct pollfd;

class IControl
{
public:
	enum Type
	{
		INT   = 0,
		FLOAT = 1,
	};

	union value_t
	{
		int32_t i;
		float   f;
	};

	virtual ~IControl() = 0;

	virtual Type getType() const = 0;

	virtual const char *getName() const = 0;

	virtual value_t getLow() const = 0;
	virtual value_t getHigh() const = 0;

	virtual int getChannelCount() const = 0;

	virtual int setValue(value_t value, int ch) = 0;
	virtual value_t getValue(int ch) const = 0;
};

class IControlServer
{
public:
	class IListener
	{
	public:
		virtual void onControlChange(IControl *control, int ch) = 0;
	};

	virtual ~IControlServer() = 0;

	virtual void setListener(IListener *listener) = 0;

	virtual int subscribe() = 0;

	virtual size_t getNumFds() const = 0;
	virtual int fillFds(struct pollfd *fds, size_t n) const = 0;
	virtual int handleFdEvents(struct pollfd *fds, size_t nfds, size_t nevents) = 0;
};

#endif // PISOUND_MICRO_MAPPER_CONTROL_SERVER_H
