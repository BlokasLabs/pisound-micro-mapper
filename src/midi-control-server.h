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

#ifndef PISOUND_MICRO_MIDI_CONTROL_SERVER_H
#define PISOUND_MICRO_MIDI_CONTROL_SERVER_H

#include "control-server.h"

#include <alsa/asoundlib.h>

#include <map>

enum MidiControlType
{
	MCT_UNKNOWN=-1,
	MCT_NOTE, // React to both On and Off.
	MCT_NOTE_ON,
	MCT_NOTE_OFF,
	MCT_CONTROL_CHANGE,
	MCT_PITCH_BEND,
	MCT_PROGRAM_CHANGE,
	MCT_CHANNEL_PRESSURE,
	MCT_POLY_AFTERTOUCH,
	MCT_START,
	MCT_CONTINUE,
	MCT_STOP,
	MCT_RESET,
};

class MidiControlServer : public IControlServer
{
public:
	MidiControlServer();
	virtual ~MidiControlServer();

	int init(const char *name);

	virtual void setListener(IListener *listener) override;

	virtual int subscribe() override;

	virtual size_t getNumFds() const override;
	virtual int fillFds(struct pollfd *fds, size_t n) const override;
	virtual int handleFdEvents(struct pollfd *fds, size_t nfds, size_t nevents) override;

	IControl *registerControl(const char *name, MidiControlType type, int8_t channel, int8_t id);

private:
	void handleEvent(const snd_seq_event_t *ev);
	int sendEvent(MidiControlType type, int8_t channel, int8_t id, int16_t value);

	class Control;
	typedef int16_t control_id_t;

	static control_id_t buildId(MidiControlType type, int8_t channel, int8_t id);
	static std::pair<control_id_t, control_id_t> identify(int16_t &value, const snd_seq_event_t *ev);

	void uninit();

	IListener *m_listener;
	snd_seq_t *m_seq;
	int       m_port;

	std::multimap<control_id_t, Control> m_controls;
};

#endif // PISOUND_MICRO_MIDI_CONTROL_SERVER_H
