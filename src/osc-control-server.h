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

#ifndef PISOUND_MICRO_OSC_CONTROL_SERVER_H
#define PISOUND_MICRO_OSC_CONTROL_SERVER_H

#include "control-server.h"

#include <lo/lo_types.h>

#include <vector>
#include <map>

namespace lo
{
	class Server;
	class Address;
}

enum OscType
{
	OSC_TYPE_UNKNOWN,
	OSC_TYPE_INT,
	OSC_TYPE_FLOAT,
};

union osc_value_t
{
	int32_t i;
	float f;
};

class OscControlServer : public IControlServer
{
public:
	OscControlServer();
	virtual ~OscControlServer();

	int init(const char *name, const char *addr);

	virtual void setListener(IListener *listener) override;

	virtual int subscribe() override;

	virtual size_t getNumFds() const override;
	virtual int fillFds(struct pollfd *fds, size_t n) const override;
	virtual int handleFdEvents(struct pollfd *fds, size_t nfds, size_t nevents) override;

	int addNotify(const char *addr);

	IControl *registerFloatControl(const char *name, const char *path, float low, float high);
	IControl *registerIntControl(const char *name, const char *path, int low, int high);

private:
	template <typename T> IControl *registerControl(const char *name, const char *path, T low, T high);

	class Control;

	void notifyChange(const Control &control);

	lo::Server *m_srv;
	IListener *m_listener;
	std::vector<lo_address> m_notifies;
	std::map<std::string, Control> m_controls;
};

#endif // PISOUND_MICRO_OSC_CONTROL_SERVER_H
