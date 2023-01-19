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

	virtual size_t getNumFds() const override;
	virtual int fillFds(struct pollfd *fds, size_t n) const override;
	virtual int handleFdEvents(struct pollfd *fds, size_t nfds, size_t nevents) override;

	IControl *registerControl(const char *name);
	void removeControl(IControl *ctrl);

private:
	class Control;
	struct ctl_info_t
	{
		unsigned int m_numid;
		size_t m_memberCount;
		int m_low;
		int m_high;
	};

	static int lookupInfo(ctl_info_t &info, snd_ctl_elem_id_t *id, snd_ctl_t *handle);

	std::map<unsigned int, Control> m_controls;

	IListener *m_listener;
	snd_ctl_t *m_handle;
};

class AlsaControlServer::Control : public IControl
{
public:
	Control(snd_ctl_t *handle, snd_ctl_elem_id_t *id, const ctl_info_t &info, const char *name);
	virtual ~Control();

	virtual const char *getName() const override;

	virtual int getMemberCount() const override;

	virtual int getLow() const override;
	virtual int getHigh() const override;

	virtual int setValue(int value, int index) override;
	virtual int getValue(int index) const override;

private:
	snd_ctl_t *m_handle;
	snd_ctl_elem_id_t *m_id;
	const ctl_info_t m_info;
	const char *m_name;
};

#endif // PISOUND_MICRO_ALSA_CONTROL_SERVER_H
