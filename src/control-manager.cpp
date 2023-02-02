#include "control-manager.h"

#include "logger.h"

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <poll.h>

static int calc(int value, int src_low, int src_high, int dst_low, int dst_high)
{
	return (int)(((float)(value - src_low) / (src_high - src_low)) * (dst_high - dst_low) + dst_low);
}

ControlManager::map_options_t ControlManager::defaultMapOptions()
{
	map_options_t opts;

	opts.m_index = -1;

	return opts;
}

ControlManager::ControlManager()
{
}

void ControlManager::addControlServer(std::shared_ptr<IControlServer> server)
{
	m_ctrlServers.push_back({ server, 0 });
	server->setListener(this);
}

void ControlManager::map(IControl &from, IControl &to, const map_options_t &opts)
{
	m_mappings.insert(std::make_pair(&from, mapping_t { &from, &to, opts }));
	int v = calc(to.getValue(opts.m_index), to.getLow(), to.getHigh(), from.getLow(), from.getHigh());
	from.setValue(v, opts.m_index);
	//maskControlChangeEvent(&from, v, opts.m_index);
}

int ControlManager::subscribe()
{
	for (auto &itr : m_ctrlServers)
	{
		int err = itr.m_server->subscribe();
		if (err < 0)
		{
			LOG_ERROR("IControlServer::subscribe failed! (%d)", err);
			return err;
		}
	}

	return 0;
}

size_t ControlManager::getNumFds() const
{
	size_t n=0;
	for (const auto &itr : m_ctrlServers)
		n += itr.m_server->getNumFds();
	return n;
}

int ControlManager::fillFds(struct pollfd *fds, size_t n) const
{
	size_t total = 0;
	for (const auto &itr : m_ctrlServers)
	{
		int cnt = itr.m_server->fillFds(fds, n);
		if (cnt < 0)
		{
			LOG_ERROR("IControlServer::fillFds failed! (%d)", cnt);
			return cnt;
		}
		assert(cnt <= n);

		total += cnt;
		n -= cnt;
		fds += cnt;

		itr.m_numFds = cnt;
	}
	return total;
}

int ControlManager::handleFdEvents(struct pollfd *fds, size_t nfds, size_t nevents)
{
	size_t total;
	for (auto &itr : m_ctrlServers)
	{
		int n = itr.m_server->handleFdEvents(fds, itr.m_numFds, nevents);
		fds += itr.m_numFds;

		if (n < 0)
		{
			LOG_ERROR("IControlServer::handleFdEvents failed! (%d)", n);
			return n;
		}

		assert(n <= nevents);

		total += n;

		nevents -= n;

		if (nevents == 0)
			break;
	}

	return total;
}

void ControlManager::onControlChange(IControl *from)
{
	LOG_DEBUG("Control %s changed! Value: %d", from->getName(), from->getValue(-1));

	int src_low = from->getLow();
	int src_high = from->getHigh();

	auto items = m_mappings.equal_range(from);
	for (auto itr = items.first; itr != items.second; ++itr)
	{
		int idx = itr->second.m_opts.m_index;

		IControl *to = itr->second.m_to;
		int dst_low = to->getLow();
		int dst_high = to->getHigh();

		int fromValue = from->getValue(idx);

		bool masked = isControlChangeEventMasked(from, fromValue, idx);
		unmaskControlChangeEvent(from);

		if (masked)
		{
			LOG_DEBUG("Ignoring masked event.");
			continue;
		}

		int toValue = calc(fromValue, src_low, src_high, dst_low, dst_high);
		int res = to->setValue(toValue, idx);
		maskControlChangeEvent(to, toValue, idx);
		LOG_DEBUG("v=%d (%d, %d, %d, %d, %d) -> %d", toValue, fromValue, src_low, src_high, dst_low, dst_high, res);

		//for (int idx=0; idx<to->getMemberCount(); ++idx)
		//	to->setValue(v, idx);
	}
}

void ControlManager::maskControlChangeEvent(IControl *control, int value, int index)
{
	m_maskedControlChangeEvents.push_back(masked_control_change_event_t { control, value, index });
}

bool ControlManager::isControlChangeEventMasked(IControl *control, int value, int index) const
{
	return std::find(m_maskedControlChangeEvents.begin(), m_maskedControlChangeEvents.end(), masked_control_change_event_t { control, value, index }) != m_maskedControlChangeEvents.end();
}

void ControlManager::unmaskControlChangeEvent(IControl *control)
{
	switch (m_maskedControlChangeEvents.size())
	{
	case 1:
		m_maskedControlChangeEvents.pop_back();
		// Fallthrough intentional.
	case 0:
		return;
	default:
		break;
	}

	auto item = std::find_if(
		m_maskedControlChangeEvents.begin(), m_maskedControlChangeEvents.end(),
		[control](const masked_control_change_event_t &e)
		{
			return control == e.m_control;
		});

	std::iter_swap(item, m_maskedControlChangeEvents.rbegin());
	m_maskedControlChangeEvents.pop_back();
}
