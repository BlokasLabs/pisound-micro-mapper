#ifndef PISOUND_MICRO_CONTROL_SERVER_H
#define PISOUND_MICRO_CONTROL_SERVER_H

#include <map>

#include "control-server.h"

namespace upisnd
{
	class Element;
}

class PisoundMicroControlServer : public IControlServer
{
public:
	PisoundMicroControlServer();
	virtual ~PisoundMicroControlServer();

	virtual void setListener(IListener *listener) override;

	virtual int subscribe() override;

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

#endif // PISOUND_MICRO_CONTROL_SERVER_H
