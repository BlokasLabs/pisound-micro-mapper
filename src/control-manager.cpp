// SPDX-License-Identifier: GPL-2.0-only
//
// pisound-micro-mapper - a daemon to facilitate Pisound Micro control mappings.
//  Copyright (C) 2023-2025  Vilniaus Blokas UAB, https://blokas.io/
//
// This file is part of pisound-micro-mapper.
//
// pisound-micro-mapper is free software: you can redistribute it and/or modify it under the terms of the
// GNU General Public License as published by the Free Software Foundation, version 2.0 of the License.
//
// pisound-micro-mapper is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even
// the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
// for more details.
//
// You should have received a copy of the GNU General Public License along with pisound-micro-mapper.
// If not, see <https://www.gnu.org/licenses/>.

#include "control-manager.h"

#include "logger.h"
#include "utils.h"

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <ctime>
#include <poll.h>

ControlManager::milliseconds_t ControlManager::now()
{
	timespec ts;
	milliseconds_t ms = -1;
	if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0)
	{
		ms = ts.tv_sec * 1000u + (ts.tv_nsec / 1000000u);
	}

	return ms;
}

template <typename SRC>
static IControl::value_t calc(float value, SRC src_low, SRC src_high, int dst_low, int dst_high)
{
	return { .i = (int)(((value - src_low) / (src_high - src_low)) * (dst_high - dst_low) + dst_low) };
}

template <typename SRC>
static IControl::value_t calc(float value, SRC src_low, SRC src_high, float dst_low, float dst_high)
{
	return { .f = (((value - src_low) / (src_high - src_low)) * (dst_high - dst_low) + dst_low) };
}

ControlManager::map_options_t ControlManager::defaultMapOptions()
{
	map_options_t opts;

	opts.m_src_ch = -1;
	opts.m_dst_ch = -1;

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

static IControl::value_t calc(IControl::Type src_type, IControl::Type dst_type, IControl::value_t value, IControl::value_t src_low, IControl::value_t src_high, IControl::value_t dst_low, IControl::value_t dst_high)
{
	switch ((src_type << 1) | dst_type)
	{
	case 0: // INT INT
		return calc(value.i, src_low.i, src_high.i, dst_low.i, dst_high.i);
	case 1: // INT FLOAT
		return calc(value.i, src_low.i, src_high.i, dst_low.f, dst_high.f);
	case 2: // FLOAT INT
		return calc(value.f, src_low.f, src_high.f, dst_low.i, dst_high.i);
	case 3: // FLOAT FLOAT
		return calc(value.f, src_low.f, src_high.f, dst_low.f, dst_high.f);
	}
	if (src_type == IControl::INT) return { .i = 0 };
	return { .f = 0 };
}

void ControlManager::map(IControl &src, IControl &dst, const map_options_t &opts)
{
	auto &e = m_mappings[&src];
	IControl::value_t v;
	if (opts.m_src_ch < 0)
	{
		if (opts.m_dst_ch < 0)
		{
			int n = std::max(src.getChannelCount(), dst.getChannelCount());
			for (int i=0; i<n; ++i)
			{
				int src_ch = i < src.getChannelCount() ? i : 0;
				int dst_ch = i < dst.getChannelCount() ? i : 0;
				e.insert(mapping_t { &src, &dst, src_ch, dst_ch });
				v = calc(src.getType(), dst.getType(), src.getValue(src_ch), src.getLow(), src.getHigh(), dst.getLow(), dst.getHigh());
				dst.setValue(v, dst_ch);
				LOG_DEBUG("1 Mapping %s[%d] -> %s[%d], %s -> %s", src.getName(), src_ch, dst.getName(), dst_ch, to_std_string(src.getValue(src_ch), src.getType()).c_str(), to_std_string(dst.getValue(dst_ch), dst.getType()).c_str());
			}
		}
		else
		{
			for (int i=0; i<src.getChannelCount(); ++i)
			{
				int src_ch = i;
				e.insert(mapping_t { &src, &dst, i, opts.m_dst_ch });
				v = calc(src.getType(), dst.getType(), src.getValue(src_ch), src.getLow(), src.getHigh(), dst.getLow(), dst.getHigh());
				dst.setValue(v, opts.m_dst_ch);
				LOG_DEBUG("2 Mapping %s[%d] -> %s[%d], %s -> %s", src.getName(), src_ch, dst.getName(), opts.m_dst_ch, to_std_string(src.getValue(src_ch), src.getType()).c_str(), to_std_string(dst.getValue(opts.m_dst_ch), dst.getType()).c_str());
			}
		}
	}
	else
	{
		int src_ch = opts.m_src_ch;
		if (opts.m_dst_ch < 0)
		{
			for (int i=0; i<dst.getChannelCount(); ++i)
			{
				e.insert(mapping_t { &src, &dst, src_ch, i });
				v = calc(src.getType(), dst.getType(), src.getValue(src_ch), src.getLow(), src.getHigh(), dst.getLow(), dst.getHigh());
				dst.setValue(v, opts.m_dst_ch);
				LOG_DEBUG("3 Mapping %s[%d] -> %s[%d], %s -> %s", src.getName(), src_ch, dst.getName(), opts.m_dst_ch, to_std_string(src.getValue(src_ch), src.getType()).c_str(), to_std_string(dst.getValue(opts.m_dst_ch), dst.getType()).c_str());
			}
		}
		else
		{
			e.insert(mapping_t { &src, &dst, src_ch, opts.m_dst_ch });
			v = calc(src.getType(), dst.getType(), src.getValue(src_ch), src.getLow(), src.getHigh(), dst.getLow(), dst.getHigh());
			dst.setValue(v, opts.m_dst_ch);
			LOG_DEBUG("4 Mapping %s[%d] -> %s[%d], %s -> %s", src.getName(), src_ch, dst.getName(), opts.m_dst_ch, to_std_string(src.getValue(src_ch), src.getType()).c_str(), to_std_string(dst.getValue(opts.m_dst_ch), dst.getType()).c_str());
		}
	}
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

void ControlManager::onControlChange(IControl *control, int ch)
{
	LOG_DEBUG("Control %s changed! Value: %s", control->getName(), to_std_string(control->getValue(-1), control->getType()).c_str());

	auto items = m_mappings.find(control);
	if (items == m_mappings.end())
		return;

	milliseconds_t ts = now();
	processTimeouts(ts);

	IControl::value_t src_low = control->getLow();
	IControl::value_t src_high = control->getHigh();
	IControl::value_t src_value = control->getValue(ch);

	for (auto itr = items->second.begin(); itr != items->second.end(); ++itr)
	{
		const struct mapping_t &mapping = *itr;

		if (ch != mapping.m_src_ch)
			continue;

		IControl *dst = mapping.m_dst;
		IControl::value_t dst_low = dst->getLow();
		IControl::value_t dst_high = dst->getHigh();

		bool masked = isControlChangeEventMasked(control, src_value, mapping.m_src_ch);

		if (masked)
		{
			LOG_DEBUG("Ignoring masked event.");
			continue;
		}

		IControl::value_t dst_value = calc(control->getType(), dst->getType(), src_value, src_low, src_high, dst_low, dst_high);
		int res = dst->setValue(dst_value, mapping.m_dst_ch);
		maskControlChangeEvent(ts, dst, dst_value, mapping.m_dst_ch);
		LOG_DEBUG("v=%s '%s' (%s, %s, %s, %s, %s) -> %d",
			to_std_string(dst_value, dst->getType()).c_str(),
			dst->getName(),
			to_std_string(src_value, control->getType()).c_str(),
			to_std_string(src_low, control->getType()).c_str(),
			to_std_string(src_high, control->getType()).c_str(),
			to_std_string(dst_low, dst->getType()).c_str(),
			to_std_string(dst_high, dst->getType()).c_str(),
			res);
	}

	unmaskControlChangeEvent(control, control->getValue(ch), ch);
}

void ControlManager::maskControlChangeEvent(milliseconds_t ts, IControl *control, IControl::value_t value, int ch)
{
	m_maskedControlChangeEvents.push_back(masked_control_change_event_t { ts, control, value, ch });
}

class ControlManager::MaskedControlChangeEventPredicate
{
public:
	inline MaskedControlChangeEventPredicate(const ControlManager::masked_control_change_event_t &e)
		:m_e(e)
	{
	}

	inline bool operator()(const ControlManager::masked_control_change_event_t &e) const
	{
		return m_e.m_control == e.m_control && memcmp(&m_e.m_value, &e.m_value, sizeof(m_e.m_value)) == 0 && m_e.m_ch == e.m_ch;
	}

private:
	const ControlManager::masked_control_change_event_t &m_e;
};

bool ControlManager::isControlChangeEventMasked(IControl *control, IControl::value_t value, int ch) const
{
	const MaskedControlChangeEventPredicate pred(masked_control_change_event_t { 0, control, value, ch });
	return std::find_if(m_maskedControlChangeEvents.begin(), m_maskedControlChangeEvents.end(), pred) != m_maskedControlChangeEvents.end();
}

void ControlManager::unmaskControlChangeEvent(IControl *control, IControl::value_t value, int ch)
{
	const MaskedControlChangeEventPredicate pred(masked_control_change_event_t { 0, control, value, ch });
	auto item = std::find_if(m_maskedControlChangeEvents.begin(), m_maskedControlChangeEvents.end(), pred);
	if (item != m_maskedControlChangeEvents.end())
	{
		std::iter_swap(item, m_maskedControlChangeEvents.rbegin());
		m_maskedControlChangeEvents.pop_back();
	}

	if (std::find_if(m_maskedControlChangeEvents.begin(), m_maskedControlChangeEvents.end(), pred) != m_maskedControlChangeEvents.end())
	{
		LOG_ERROR("[[[[ FOUND STILL MASKED EVENT ]]]]!!!!!!!");
	}

#if 0
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
#endif
}

void ControlManager::processTimeouts(milliseconds_t now)
{
	for (auto itr = m_maskedControlChangeEvents.begin(); itr != m_maskedControlChangeEvents.end();)
	{
		if (now - itr->m_ts >= 5)
		{
			LOG_DEBUG("Event mask for %s timeout", itr->m_control->getName());
			itr = m_maskedControlChangeEvents.erase(itr);
		}
		else ++itr;
	}
}
