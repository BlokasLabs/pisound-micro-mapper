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

#ifndef PISOUND_MICRO_MAPPER_CONTROL_SERVER_H
#define PISOUND_MICRO_MAPPER_CONTROL_SERVER_H

#include <cstddef>
#include <cstdint>

struct pollfd;

class IControl
{
public:
	enum Type
	{
		INT   = 0,
		FLOAT = 1,
	};

	union value_t
	{
		int32_t i;
		float   f;
	};

	virtual ~IControl() = 0;

	virtual Type getType() const = 0;

	virtual const char *getName() const = 0;

	virtual value_t getLow() const = 0;
	virtual value_t getHigh() const = 0;

	virtual int getChannelCount() const = 0;

	virtual int setValue(value_t value, int ch) = 0;
	virtual value_t getValue(int ch) const = 0;
};

class IControlServer
{
public:
	class IListener
	{
	public:
		virtual void onControlChange(IControl *control, int ch) = 0;
	};

	virtual ~IControlServer() = 0;

	virtual void setListener(IListener *listener) = 0;

	virtual int subscribe() = 0;

	virtual size_t getNumFds() const = 0;
	virtual int fillFds(struct pollfd *fds, size_t n) const = 0;
	virtual int handleFdEvents(struct pollfd *fds, size_t nfds, size_t nevents) = 0;
};

#endif // PISOUND_MICRO_MAPPER_CONTROL_SERVER_H
