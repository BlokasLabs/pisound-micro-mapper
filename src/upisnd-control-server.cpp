#include "upisnd-control-server.h"

#include <cstring>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>

PisoundMicroControlServer::PisoundMicroControlServer()
	:m_listener(NULL)
{
}

PisoundMicroControlServer::~PisoundMicroControlServer()
{
}

void PisoundMicroControlServer::setListener(IListener *listener)
{
	m_listener = listener;
}

size_t PisoundMicroControlServer::getNumFds() const
{
	return m_controls.size();
}

int PisoundMicroControlServer::fillFds(struct pollfd *fds, size_t n) const
{
	if (m_controls.size() > n)
		return -EINVAL;

	for (const auto &itr : m_controls)
	{
		fds->fd = itr.first;
		fds->events = POLLPRI;
		fds->revents = 0;
		++fds;
	}

	return m_controls.size();
}

int PisoundMicroControlServer::handleFdEvents(struct pollfd *fds, size_t nfds, size_t nevents)
{
	size_t i=0, j=0;
	while (i < nfds && j < nevents)
	{
		printf("fd %d events %04x Revents %04x\n", fds[i].fd, fds[i].events, fds[i].revents);
		if (fds[i].revents & POLLPRI)
		{
			if (m_listener)
			{
				auto item = m_controls.find(fds[i].fd);
				if (item != m_controls.end())
				{
					m_listener->onControlChange(&item->second);
				}
			}
			++j;
		}
		++i;
	}

	return j;
}

IControl *PisoundMicroControlServer::registerControl(upisnd::Element element)
{
	if (findByName(element.getName()) != m_controls.end())
	{
		errno = EEXIST;
		return NULL;
	}

	upisnd::ValueFd fd = element.openValueFd(O_CLOEXEC | O_RDWR);
	if (!fd.isValid())
		return NULL;

	auto r = m_controls.emplace(std::piecewise_construct, std::forward_as_tuple(fd.get()), std::forward_as_tuple(std::move(element), std::move(fd)));

	return &r.first->second;
}

void PisoundMicroControlServer::removeControl(IControl *control)
{
	for (auto itr = m_controls.begin(); itr != m_controls.end(); ++itr)
	{
		if (control == &itr->second)
		{
			m_controls.erase(itr);
			return;
		}
	}
}

std::map<int, PisoundMicroControlServer::Control>::iterator PisoundMicroControlServer::findByName(const char *name)
{
	if (name && *name)
	{
		size_t n = strlen(name)+1;
		for (auto itr = m_controls.begin(); itr != m_controls.end(); ++itr)
		{
			if (strncmp(name, itr->second.getName(), n) == 0)
				return itr;
		}
	}

	return m_controls.end();
}

PisoundMicroControlServer::Control::Control(upisnd::Element &&element, upisnd::ValueFd &&fd)
	:m_element(std::move(element))
	,m_value(std::move(fd))
{
}

const char *PisoundMicroControlServer::Control::getName() const
{
	return m_element.getName();
}

int PisoundMicroControlServer::Control::getMemberCount() const
{
	return 1;
}

int PisoundMicroControlServer::Control::getLow() const
{
	switch (m_element.getType())
	{
	default:
	case UPISND_ELEMENT_TYPE_INVALID:
	case UPISND_ELEMENT_TYPE_NONE:
	case UPISND_ELEMENT_TYPE_GPIO:
	case UPISND_ELEMENT_TYPE_ACTIVITY_LED:
		return 0;
	case UPISND_ELEMENT_TYPE_ENCODER:
		{
			upisnd_encoder_opts_t opts;
			m_element.as<upisnd::Encoder>().getOpts(opts);
			return opts.value_range.low;
		}
	case UPISND_ELEMENT_TYPE_ANALOG_INPUT:
		{
			upisnd_analog_input_opts_t opts;
			m_element.as<upisnd::AnalogInput>().getOpts(opts);
			return opts.value_range.low;
		}
	}
}

int PisoundMicroControlServer::Control::getHigh() const
{
	switch (m_element.getType())
	{
	default:
	case UPISND_ELEMENT_TYPE_INVALID:
	case UPISND_ELEMENT_TYPE_NONE:
	case UPISND_ELEMENT_TYPE_ACTIVITY_LED:
		return 0;
	case UPISND_ELEMENT_TYPE_GPIO:
		return 1;
	case UPISND_ELEMENT_TYPE_ENCODER:
		{
			upisnd_encoder_opts_t opts;
			m_element.as<upisnd::Encoder>().getOpts(opts);
			return opts.value_range.high;
		}
	case UPISND_ELEMENT_TYPE_ANALOG_INPUT:
		{
			upisnd_analog_input_opts_t opts;
			m_element.as<upisnd::AnalogInput>().getOpts(opts);
			return opts.value_range.high;
		}
	}
}

int PisoundMicroControlServer::Control::setValue(int value, int index)
{
	(void)index;
	return m_value.write(value);
}

int PisoundMicroControlServer::Control::getValue(int index) const
{
	(void)index;
	return m_value.read();
}
