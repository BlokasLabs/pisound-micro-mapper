#include "upisnd-control-server.h"

#include "logger.h"

#include <cstring>
#include <cstdio>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>

#include <pisound-micro.h>

class PisoundMicroControlServer::Control : public IControl
{
public:
	Control(upisnd::Element &&element, upisnd::ValueFd &&fd);

	virtual const char *getName() const override;

	virtual Type getType() const override;

	virtual int getChannelCount() const override;

	virtual value_t getLow() const override;
	virtual value_t getHigh() const override;

	virtual int setValue(value_t value, int index) override;
	virtual value_t getValue(int index) const override;

private:
	upisnd::Element m_element;
	upisnd::ValueFd m_value;
};

PisoundMicroControlServer::PisoundMicroControlServer()
	:m_listener(NULL)
{
	upisnd_init();
}

PisoundMicroControlServer::~PisoundMicroControlServer()
{
	m_fdlessControls.clear();
	m_controls.clear();
	upisnd_uninit();
}

void PisoundMicroControlServer::setListener(IListener *listener)
{
	m_listener = listener;
}

int PisoundMicroControlServer::subscribe()
{
	// Read out all values, to clear any pending underlying FD events.
	for (const auto &itr : m_controls)
	{
		itr.second.getValue(-1);
	}

	return 0;
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
		LOG_DEBUG("fd %d events %04x Revents %04x", fds[i].fd, fds[i].events, fds[i].revents);
		if (fds[i].revents & POLLPRI)
		{
			if (m_listener)
			{
				auto item = m_controls.find(fds[i].fd);
				if (item != m_controls.end())
				{
					m_listener->onControlChange(&item->second, 0);
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
	if (findByName(element.getName()) != m_controls.end() || findByNameFdless(element.getName()) != m_fdlessControls.end())
	{
		errno = EEXIST;
		return NULL;
	}

	int access_flag;

	switch (element.getType())
	{
	case UPISND_ELEMENT_TYPE_ENCODER:
		access_flag = O_RDWR;
		break;
	case UPISND_ELEMENT_TYPE_GPIO:
		if (element.as<upisnd::Gpio>().getDirection() == UPISND_PIN_DIR_OUTPUT)
			access_flag = O_RDWR;
		else access_flag = O_RDONLY;
		break;
	case UPISND_ELEMENT_TYPE_ANALOG_INPUT:
		access_flag = O_RDONLY;
		break;
	default:
		access_flag = -1;
		break;
	}

	upisnd::ValueFd fd;
	if (access_flag >= 0)
	{
		fd = element.openValueFd(O_CLOEXEC | access_flag);
		if (!fd.isValid())
			return NULL;
		auto r = m_controls.emplace(std::piecewise_construct, std::forward_as_tuple(fd.get()), std::forward_as_tuple(std::move(element), std::move(fd)));
		return &r.first->second;
	}
	else
	{
		m_fdlessControls.emplace_back(std::move(element), std::move(fd));
		return &m_fdlessControls.back();
	}
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

std::map<int, PisoundMicroControlServer::Control>::const_iterator PisoundMicroControlServer::findByName(const char *name) const
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

std::list<PisoundMicroControlServer::Control>::const_iterator PisoundMicroControlServer::findByNameFdless(const char *name) const
{
	if (name && *name)
	{
		size_t n = strlen(name)+1;
		for (auto itr = m_fdlessControls.begin(); itr != m_fdlessControls.end(); ++itr)
		{
			if (strncmp(name, itr->getName(), n) == 0)
				return itr;
		}
	}

	return m_fdlessControls.end();
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

IControl::Type PisoundMicroControlServer::Control::getType() const
{
	return IControl::INT;
}

int PisoundMicroControlServer::Control::getChannelCount() const
{
	return 1;
}

IControl::value_t PisoundMicroControlServer::Control::getLow() const
{
	switch (m_element.getType())
	{
	default:
	case UPISND_ELEMENT_TYPE_INVALID:
	case UPISND_ELEMENT_TYPE_NONE:
	case UPISND_ELEMENT_TYPE_GPIO:
	case UPISND_ELEMENT_TYPE_ACTIVITY_LED:
		return { .i = 0 };
	case UPISND_ELEMENT_TYPE_ENCODER:
		{
			upisnd_encoder_opts_t opts;
			m_element.as<upisnd::Encoder>().getOpts(opts);
			return { .i = opts.value_range.low };
		}
	case UPISND_ELEMENT_TYPE_ANALOG_INPUT:
		{
			upisnd_analog_input_opts_t opts;
			m_element.as<upisnd::AnalogInput>().getOpts(opts);
			return { .i = opts.value_range.low };
		}
	}
}

IControl::value_t PisoundMicroControlServer::Control::getHigh() const
{
	switch (m_element.getType())
	{
	default:
	case UPISND_ELEMENT_TYPE_INVALID:
	case UPISND_ELEMENT_TYPE_NONE:
	case UPISND_ELEMENT_TYPE_ACTIVITY_LED:
		return { .i = 0 };
	case UPISND_ELEMENT_TYPE_GPIO:
		return { .i = 1 };
	case UPISND_ELEMENT_TYPE_ENCODER:
		{
			upisnd_encoder_opts_t opts;
			m_element.as<upisnd::Encoder>().getOpts(opts);
			return { .i = opts.value_range.high };
		}
	case UPISND_ELEMENT_TYPE_ANALOG_INPUT:
		{
			upisnd_analog_input_opts_t opts;
			m_element.as<upisnd::AnalogInput>().getOpts(opts);
			return { .i = opts.value_range.high };
		}
	}
}

int PisoundMicroControlServer::Control::setValue(IControl::value_t value, int index)
{
	(void)index;
	return m_value.write(value.i);
}

IControl::value_t PisoundMicroControlServer::Control::getValue(int index) const
{
	(void)index;
	return { .i = m_value.read() };
}
