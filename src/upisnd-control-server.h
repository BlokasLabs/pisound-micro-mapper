#ifndef PISOUND_MICRO_CONTROL_SERVER_H
#define PISOUND_MICRO_CONTROL_SERVER_H

#include <map>
#include <pisound-micro.h>

#include "control-server.h"

class PisoundMicroControlServer : public IControlServer
{
public:
	PisoundMicroControlServer();
	virtual ~PisoundMicroControlServer();

	virtual void setListener(IListener *listener) override;

	virtual size_t getNumFds() const override;
	virtual int fillFds(struct pollfd *fds, size_t n) const override;
	virtual int handleFdEvents(struct pollfd *fds, size_t nfds, size_t nevents) override;

	IControl *registerControl(upisnd::Element element);
	void removeControl(IControl *control);

private:
	class Control;

	std::map<int, Control>::iterator findByName(const char *name);

	IListener *m_listener;
	std::map<int, Control> m_controls;
};

class PisoundMicroControlServer::Control : public IControl
{
public:
	Control(upisnd::Element &&element, upisnd::ValueFd &&fd);

	virtual const char *getName() const override;

	virtual int getLow() const override;
	virtual int getHigh() const override;

	virtual int setValue(int value, int index) override;
	virtual int getValue(int index) const override;

private:
	upisnd::Element m_element;
	upisnd::ValueFd m_value;
};

#endif // PISOUND_MICRO_CONTROL_SERVER_H
