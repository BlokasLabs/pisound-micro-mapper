#ifndef PISOUND_MICRO_MAPPER_CONTROL_SERVER_H
#define PISOUND_MICRO_MAPPER_CONTROL_SERVER_H

#include <cstddef>

class IControl
{
public:
	virtual ~IControl() = 0;

	virtual const char *getName() const = 0;

	virtual int getLow() const = 0;
	virtual int getHigh() const = 0;

	virtual int getMemberCount() const = 0;

	virtual int setValue(int value, int index) = 0;
	virtual int getValue(int index) const = 0;
};

class IControlServer
{
public:
	class IListener
	{
	public:
		virtual void onControlChange(IControl *control) = 0;
	};

	virtual ~IControlServer() = 0;

	virtual void setListener(IListener *listener) = 0;

	virtual size_t getNumFds() const = 0;
	virtual int fillFds(struct pollfd *fds, size_t n) const = 0;
	virtual int handleFdEvents(struct pollfd *fds, size_t nfds, size_t nevents) = 0;
};

#endif // PISOUND_MICRO_MAPPER_CONTROL_SERVER_H
