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

#ifndef PISOUND_MICRO_CONTROL_SERVER_H
#define PISOUND_MICRO_CONTROL_SERVER_H

#include <map>
#include <list>

#include "control-server.h"

namespace upisnd
{
	class Element;
}

class PisoundMicroControlServer : public IControlServer
{
public:
	PisoundMicroControlServer();
	virtual ~PisoundMicroControlServer();

	virtual void setListener(IListener *listener) override;

	virtual int subscribe() override;

	virtual size_t getNumFds() const override;
	virtual int fillFds(struct pollfd *fds, size_t n) const override;
	virtual int handleFdEvents(struct pollfd *fds, size_t nfds, size_t nevents) override;

	IControl *registerControl(upisnd::Element element);
	void removeControl(IControl *control);

private:
	class Control;

	std::map<int, Control>::const_iterator findByName(const char *name) const;
	std::list<Control>::const_iterator findByNameFdless(const char *name) const;

	IListener *m_listener;
	std::map<int, Control> m_controls;
	std::list<Control> m_fdlessControls;
};

#endif // PISOUND_MICRO_CONTROL_SERVER_H
