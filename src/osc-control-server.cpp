#include <lo/lo_cpp.h>

#include <poll.h>
#include <errno.h>

#include "osc-control-server.h"
#include "logger.h"

template <typename T> T clamp(T value, T low, T high)
{
	T min, max;
	if (low <= high)
	{
		min = low;
		max = high;
	}
	else
	{
		min = high;
		max = low;
	}

	if (value <= min)
		return min;
	else if (value >= max)
		return max;
	else
		return value;
}

class OscControlServer::Control : public IControl
{
public:
	Control(OscControlServer &srv, std::string name, std::string path, float low, float high)
		:m_srv(srv)
		,m_name(name)
		,m_path(path)
		,m_type(OSC_TYPE_FLOAT)
	{
		m_value.f = 0.;
		m_low.f   = low;
		m_high.f  = high;
	}

	Control(OscControlServer &srv, std::string name, std::string path, int low, int high)
		:m_srv(srv)
		,m_name(name)
		,m_path(path)
		,m_type(OSC_TYPE_INT)
	{
		m_value.i = 0;
		m_low.i   = low;
		m_high.i  = high;
	}


	static int oscMethodHandler(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data)
	{
		Control *c = (Control*)user_data;

		if (argc != 1)
			return 1;

		if (c->m_type == OSC_TYPE_FLOAT)
		{
			c->m_value.f = clamp(argv[0]->f, c->m_low.f, c->m_high.f);
		}
		else
		{
			c->m_value.i = clamp(argv[0]->i, c->m_low.i, c->m_high.i);
		}

		c->m_srv.m_listener->onControlChange(c);

		return 0;
	}

	virtual const char *getName() const override
	{
		return m_name.c_str();
	}

	virtual value_t getLow() const override
	{
		value_t v;
		if (m_type == OSC_TYPE_FLOAT)
			v.f = m_low.f;
		else
			v.i = m_low.i;
		return v;
	}

	virtual value_t getHigh() const override
	{
		value_t v;
		if (m_type == OSC_TYPE_FLOAT)
			v.f = m_high.f;
		else
			v.i = m_high.i;
		return v;
	}

	virtual Type getType() const override
	{
		return m_type == OSC_TYPE_FLOAT ? FLOAT : INT;
	}

	virtual int getMemberCount() const override
	{
		return 1;
	}

	virtual int setValue(value_t value, int index) override
	{
		if (m_type == OSC_TYPE_FLOAT)
			m_value.f = value.f;
		else
			m_value.i = value.i;
		m_srv.notifyChange(*this);
		return 0;
	}

	virtual value_t getValue(int index) const override
	{
		value_t v;
		if (m_type == OSC_TYPE_FLOAT)
			v.f = m_value.f;
		else
			v.i = m_value.i;
		return v;
	}

	const char *getPath() const
	{
		return m_path.c_str();
	}

#if 0
	const char *getTypeStr() const
	{
		return m_type == OSC_TYPE_FLOAT ? "f" : "i";
	}
#endif

private:
	OscControlServer &m_srv;
	std::string      m_name;
	std::string      m_path;
	OscType          m_type;
	osc_value_t      m_value;
	osc_value_t      m_low;
	osc_value_t      m_high;
};

static void osc_error_handler(int num, const char *msg, const char *where)
{
	Logger::error("OSC error %d in %s: %s\n", num, where, msg);
}

OscControlServer::OscControlServer()
	:m_srv(NULL)
	,m_listener(NULL)
{
}

OscControlServer::~OscControlServer()
{
	for (auto a : m_notifies)
		lo_address_free(a);
	m_notifies.clear();
	if (m_srv)
	{
		delete m_srv;
		m_srv = NULL;
	}
}

int OscControlServer::init(const char *name, const char *addr)
{
	if (addr)
	{
		lo_server s = lo_server_new_from_url(addr, &osc_error_handler);
		if (!s)
		{
			Logger::error("OSC server init failed!\n");
			return -EINVAL;
		}

		m_srv = new lo::Server(s);

		Logger::info("OSC server '%s' listening on '%s'!\n", name, addr);
	}
	else
	{
		Logger::error("OSC server '%s' initialized for notifications only.\n");
	}

	return 0;
}

void OscControlServer::setListener(IListener *listener)
{
	m_listener = listener;
}

int OscControlServer::subscribe()
{
	return 0;
}

size_t OscControlServer::getNumFds() const
{
	return m_srv ? 1 : 0;
}

int OscControlServer::fillFds(struct pollfd *fds, size_t n) const
{
	if (!m_srv)
		return 0;

	fds->fd = m_srv->socket_fd();
	fds->events = POLLIN;
	fds->revents = 0;

	return 1;
}

int OscControlServer::handleFdEvents(struct pollfd *fds, size_t nfds, size_t nevents)
{
	if (!nevents || !m_srv)
		return 0;

	size_t i=0,j=0;
	while (i<nfds && j<nevents)
	{
		if (fds[i].revents & POLLIN)
		{
			while (m_srv->recv(0) != 0)
				++j;
		}
		++i;
	}

	return i;
}

int OscControlServer::addNotify(const char *addr)
{
	if (!addr)
		return -EINVAL;

	lo_address a = lo_address_new_from_url(addr);
	if (!a)
		return -EINVAL;

	m_notifies.push_back(a);
	return 0;
}

template <typename T> static const char* get_osc_type_string();

template <> const char* get_osc_type_string<float>()
{
	return "f";
}

template <> const char* get_osc_type_string<int>()
{
	return "i";
}

template <typename T> IControl *OscControlServer::registerControl(const char *name, const char *path, T low, T high)
{
	if (m_controls.find(name) != m_controls.end())
	{
		errno = EEXIST;
		return NULL;
	}

	auto c = m_controls.emplace(std::piecewise_construct, std::forward_as_tuple(name), std::forward_as_tuple(*this, name, path, low, high));

	if (m_srv)
	{
		m_srv->add_method(lo::string_type(path), lo::string_type(get_osc_type_string<T>()), &Control::oscMethodHandler, &c.first->second);
	}

	return &c.first->second;
}

IControl *OscControlServer::registerFloatControl(const char *name, const char *path, float low, float high)
{
	return registerControl(name, path, low, high);
}

IControl *OscControlServer::registerIntControl(const char *name, const char *path, int low, int high)
{
	return registerControl(name, path, low, high);
}

void OscControlServer::notifyChange(const Control &control)
{
	if (m_srv)
	{
		for (auto a : m_notifies)
		{
			lo::Address addr(a, false);
			if (control.getType() == IControl::FLOAT)
				addr.send_from(*m_srv, control.getPath(), "f", control.getValue(0).f);
			else
			{
				addr.send_from(*m_srv, control.getPath(), "i", control.getValue(0).i);
			}
		}
	}
	else
	{
		for (auto a : m_notifies)
		{
			lo::Address addr(a, false);
			if (control.getType() == IControl::FLOAT)
				addr.send(control.getPath(), "f", control.getValue(0).f);
			else
				addr.send(control.getPath(), "i", control.getValue(0).i);
		}
	}
}
