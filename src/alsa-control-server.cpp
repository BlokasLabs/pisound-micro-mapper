#include "alsa-control-server.h"

#include <errno.h>

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
	int err = snd_ctl_open(&m_handle, name, 0);

	if (err != 0)
		return err;

	return snd_ctl_subscribe_events(m_handle, 1);
}

void AlsaControlServer::setListener(IListener *listener)
{
	m_listener = listener;
}

size_t AlsaControlServer::getNumFds() const
{
	return snd_ctl_poll_descriptors_count(m_handle);
}

int AlsaControlServer::fillFds(struct pollfd *fds, size_t n) const
{
	if (n < getNumFds())
		return -EINVAL;

	return snd_ctl_poll_descriptors(m_handle, fds, n);
}

int AlsaControlServer::handleFdEvents(struct pollfd *fds, size_t nfds, size_t nevents)
{
	if (!nevents)
		return 0;

	unsigned short revents = 0;
	int err = snd_ctl_poll_descriptors_revents(m_handle, fds, getNumFds(), &revents);
	if (err < 0)
		return err;

	if (revents)
	{
		snd_ctl_event_t *event;
		snd_ctl_event_alloca(&event);

		err = snd_ctl_read(m_handle, event);

		if (err >= 1 && m_listener)
		{
			if (snd_ctl_event_get_type(event) == SND_CTL_EVENT_ELEM)
			{
				unsigned int numid = snd_ctl_event_elem_get_numid(event);
				auto item = m_controls.find(numid);
				if (item != m_controls.end())
					m_listener->onControlChange(&item->second);
			}
		}

		return err;
	}

	return 0;
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

void AlsaControlServer::removeControl(IControl *ctrl)
{
	// todo
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

int AlsaControlServer::Control::getMemberCount() const
{
	return m_info.m_memberCount;
}

int AlsaControlServer::Control::getLow() const
{
	return m_info.m_low;
}

int AlsaControlServer::Control::getHigh() const
{
	return m_info.m_high;
}

int lookup_id(snd_ctl_elem_id_t *id, snd_ctl_t *handle)
{
	int err;
	snd_ctl_elem_info_t *info;
	snd_ctl_elem_info_alloca(&info);

	snd_ctl_elem_info_set_id(info, id);
	if ((err = snd_ctl_elem_info(handle, info)) < 0) {
		fprintf(stderr, "Cannot find the given element from card\n");
		return err;
	}
	snd_ctl_elem_info_get_id(info, id);

	return 0;
}

int AlsaControlServer::Control::setValue(int value, int index)
{
	snd_ctl_elem_value_t *v, *o;
	snd_ctl_elem_value_alloca(&v);
	snd_ctl_elem_value_alloca(&o);
	//snd_ctl_elem_value_set_id(v, m_id);

	snd_ctl_elem_lock(m_handle, m_id);

	snd_ctl_elem_value_set_numid(o, m_info.m_numid);

	snd_ctl_elem_read(m_handle, o);

	int oldval[2];
	oldval[0] = snd_ctl_elem_value_get_integer(o, 0);
	oldval[1] = snd_ctl_elem_value_get_integer(o, 1);

	for (size_t i=0; i<m_info.m_memberCount; ++i)
	{
		if (index < 0 || index >= m_info.m_memberCount || index == i)
		{
			printf("Setting new value[%d] -> %d\n", i, value);
			snd_ctl_elem_value_set_integer(v, i, value);
		}
		else
		{
			printf("Reusing old value[%d] -> %d\n", i, oldval[i]);
			snd_ctl_elem_value_set_integer(v, i, oldval[i]+1);
		}

		//if (i == 0) snd_ctl_elem_value_set_integer(v, i, value);
		//else snd_ctl_elem_value_set_integer(v, i, 255-value);
	}

	//if (index < 0 || index >= m_info.m_memberCount)
	//{
	//	for (size_t i=0; i<m_info.m_memberCount; ++i)
	//		snd_ctl_elem_value_set_integer(v, i, value);
	//}
	//else snd_ctl_elem_value_set_integer(v, index, value);

	snd_ctl_elem_value_set_numid(v, m_info.m_numid);

	snd_ctl_elem_write(m_handle, v);

	snd_ctl_elem_unlock(m_handle, m_id);

	return 0;
}

int AlsaControlServer::Control::getValue(int index) const
{
	snd_ctl_elem_value_t *v;
	snd_ctl_elem_value_alloca(&v);
	snd_ctl_elem_value_set_id(v, m_id);
	snd_ctl_elem_read(m_handle, v);

	if (index < 0 || index >= m_info.m_memberCount)
		index = 0;

	return snd_ctl_elem_value_get_integer(v, index);
}
