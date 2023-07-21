#ifndef PISOUND_MICRO_ALSA_CONTROL_SERVER_H
#define PISOUND_MICRO_ALSA_CONTROL_SERVER_H

#include "control-server.h"

#include <alsa/asoundlib.h>

#include <map>

class AlsaControlServer : public IControlServer
{
public:
	AlsaControlServer();
	virtual ~AlsaControlServer();

	int init(const char *device);

	virtual void setListener(IListener *listener) override;

	virtual int subscribe() override;

	virtual size_t getNumFds() const override;
	virtual int fillFds(struct pollfd *fds, size_t n) const override;
	virtual int handleFdEvents(struct pollfd *fds, size_t nfds, size_t nevents) override;

	IControl *registerControl(const char *name);

private:
	class Control;
	struct ctl_info_t;

	static int lookupInfo(ctl_info_t &info, snd_ctl_elem_id_t *id, snd_ctl_t *handle);

	std::map<unsigned int, Control> m_controls;

	IListener *m_listener;
	snd_ctl_t *m_handle;
};

#endif // PISOUND_MICRO_ALSA_CONTROL_SERVER_H
