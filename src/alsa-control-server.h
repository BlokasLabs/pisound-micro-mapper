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

	virtual int subscribe() override;

	virtual size_t getNumFds() const override;
	virtual int fillFds(struct pollfd *fds, size_t n) const override;
	virtual int handleFdEvents(struct pollfd *fds, size_t nfds, size_t nevents) override;

	IControl *registerControl(const char *name);

private:
	class Control;
	struct ctl_info_t;

	static int lookupInfo(ctl_info_t &info, snd_ctl_elem_id_t *id, snd_ctl_t *handle);

	std::map<unsigned int, Control> m_controls;

	IListener *m_listener;
	snd_ctl_t *m_handle;
};

#endif // PISOUND_MICRO_ALSA_CONTROL_SERVER_H
