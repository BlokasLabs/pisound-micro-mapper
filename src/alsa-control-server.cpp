#include "alsa-control-server.h"
#include "logger.h"

#include <errno.h>

struct AlsaControlServer::ctl_info_t
{
	unsigned int m_numid;
	size_t m_memberCount;
	int m_low;
	int m_high;
};

class AlsaControlServer::Control : public IControl
{
public:
	Control(snd_ctl_t *handle, snd_ctl_elem_id_t *id, const ctl_info_t &info, const char *name);
	virtual ~Control();

	virtual const char *getName() const override;

	virtual Type getType() const override;

	virtual int getMemberCount() const override;

	virtual value_t getLow() const override;
	virtual value_t getHigh() const override;

	virtual int setValue(value_t value, int index) override;
	virtual value_t getValue(int index) const override;

private:
	snd_ctl_t *m_handle;
	snd_ctl_elem_id_t *m_id;
	const ctl_info_t m_info;
	const char *m_name;
};

int AlsaControlServer::lookupInfo(ctl_info_t &info, snd_ctl_elem_id_t *id, snd_ctl_t *handle)
{
	int err;
	snd_ctl_elem_info_t *i;
	snd_ctl_elem_info_alloca(&i);

	snd_ctl_elem_info_set_id(i, id);
	if ((err = snd_ctl_elem_info(handle, i)) < 0)
		return err;

	snd_ctl_elem_info_get_id(i, id);
	info.m_numid = snd_ctl_elem_id_get_numid(id);
	info.m_memberCount = snd_ctl_elem_info_get_count(i);
	info.m_low = snd_ctl_elem_info_get_min(i);
	info.m_high = snd_ctl_elem_info_get_max(i);

	return 0;
}

AlsaControlServer::AlsaControlServer()
	:m_listener(NULL)
	,m_handle(NULL)
{
}

AlsaControlServer::~AlsaControlServer()
{
	if (m_handle)
	{
		snd_ctl_close(m_handle);
		m_handle = NULL;
	}
}

int AlsaControlServer::init(const char *name)
{
	return snd_ctl_open(&m_handle, name, 0);
}

void AlsaControlServer::setListener(IListener *listener)
{
	m_listener = listener;
}

int AlsaControlServer::subscribe()
{
	return snd_ctl_subscribe_events(m_handle, 1);
}

size_t AlsaControlServer::getNumFds() const
{
	return snd_ctl_poll_descriptors_count(m_handle);
}

int AlsaControlServer::fillFds(struct pollfd *fds, size_t n) const
{
	if (n < getNumFds())
	{
		LOG_ERROR("Not enough space in fds array!\n");
		return -EINVAL;
	}

	return snd_ctl_poll_descriptors(m_handle, fds, n);
}

int AlsaControlServer::handleFdEvents(struct pollfd *fds, size_t nfds, size_t nevents)
{
	if (!nevents)
		return 0;

	unsigned short *revents = (unsigned short*)alloca(sizeof(unsigned short) * nfds);
	int err = snd_ctl_poll_descriptors_revents(m_handle, fds, nfds, revents);
	if (err < 0)
		return err;

	snd_ctl_event_t *event;
	snd_ctl_event_alloca(&event);

	size_t i=0, j=0;
	while (i < nfds && j < nevents)
	{
		if (revents[i] & POLLIN)
		{
			err = snd_ctl_read(m_handle, event);

			if (err >= 1 && m_listener)
			{
				if (snd_ctl_event_get_type(event) == SND_CTL_EVENT_ELEM)
				{
					unsigned int numid = snd_ctl_event_elem_get_numid(event);
					auto item = m_controls.find(numid);
					if (item != m_controls.end())
					{
						unsigned int mask = snd_ctl_event_elem_get_mask(event);
						if (mask == SND_CTL_EVENT_MASK_REMOVE)
						{
							LOG_DEBUG("Control %d removed!", numid);
							m_controls.erase(item);
						}
						else if (mask & SND_CTL_EVENT_MASK_VALUE)
						{
							m_listener->onControlChange(&item->second);
						}
					}
				}
			}
			++j;
		}
		++i;
	}

	return j;
}

IControl *AlsaControlServer::registerControl(const char *name)
{
	int err;
	snd_ctl_elem_id_t *id;
	if ((err = snd_ctl_elem_id_malloc(&id)) < 0)
	{
		errno = -err;
		return NULL;
	}

	snd_ctl_elem_id_set_interface(id, SND_CTL_ELEM_IFACE_MIXER);
	snd_ctl_elem_id_set_name(id, name);
	ctl_info_t info;
	if ((err = lookupInfo(info, id, m_handle)) < 0)
	{
		snd_ctl_elem_id_free(id);
		errno = -err;
		return NULL;
	}

	if (m_controls.find(info.m_numid) != m_controls.end())
	{
		snd_ctl_elem_id_free(id);
		errno = EEXIST;
		return NULL;
	}

	auto r = m_controls.emplace(std::piecewise_construct, std::forward_as_tuple(info.m_numid), std::forward_as_tuple(m_handle, id, info, strdup(name)));

	return &r.first->second;
}

AlsaControlServer::Control::Control(snd_ctl_t *handle, snd_ctl_elem_id_t *id, const ctl_info_t &info, const char *name)
	:m_handle(handle)
	,m_id(id)
	,m_info(info)
	,m_name(name)
{
}

AlsaControlServer::Control::~Control()
{
	if (m_id)
	{
		snd_ctl_elem_id_free(m_id);
		m_id = NULL;
	}
	if (m_name)
	{
		free((char*)m_name);
		m_name = NULL;
	}
}

const char *AlsaControlServer::Control::getName() const
{
	return m_name;
}

IControl::Type AlsaControlServer::Control::getType() const
{
	return IControl::INT;
}

int AlsaControlServer::Control::getMemberCount() const
{
	return m_info.m_memberCount;
}

IControl::value_t AlsaControlServer::Control::getLow() const
{
	return { .i = m_info.m_low };
}

IControl::value_t AlsaControlServer::Control::getHigh() const
{
	return { .i = m_info.m_high };
}

int AlsaControlServer::Control::setValue(IControl::value_t value, int index)
{
	snd_ctl_elem_value_t *v;
	snd_ctl_elem_value_alloca(&v);
	snd_ctl_elem_value_set_id(v, m_id);

	if (index < 0 || index >= m_info.m_memberCount)
	{
		for (size_t i=0; i<m_info.m_memberCount; ++i)
			snd_ctl_elem_value_set_integer(v, i, value.i);
	}
	else snd_ctl_elem_value_set_integer(v, index, value.i);

	snd_ctl_elem_write(m_handle, v);

	return 0;
}

IControl::value_t AlsaControlServer::Control::getValue(int index) const
{
	snd_ctl_elem_value_t *v;
	snd_ctl_elem_value_alloca(&v);
	snd_ctl_elem_value_set_id(v, m_id);
	snd_ctl_elem_read(m_handle, v);

	if (index < 0 || index >= m_info.m_memberCount)
		index = 0;

	return { .i = snd_ctl_elem_value_get_integer(v, index) };
}
