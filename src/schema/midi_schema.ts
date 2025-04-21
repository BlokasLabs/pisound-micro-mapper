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

export enum MidiControlType {
	NOTE = 'note',
	NOTE_ON = 'note_on',
	NOTE_OFF = 'note_off',
	CONTROL_CHANGE = 'control_change',
	PROGRAM_CHANGE = 'program_change',
	PITCH_BEND = 'pitch_bend',
	CHANNEL_PRESSURE = 'channel_pressure',
	POLY_AFTERTOUCH = 'poly_aftertouch',
	START = 'start',
	CONTINUE = 'continue',
	STOP = 'stop',
	RESET = 'reset',
}

export interface MidiControlBase {
	type: MidiControlType,
}

export interface MidiControlWithChannel extends MidiControlBase {
	// @minimum 1
	// @maximum 16
	channel: number,
}

export interface MidiControlWithChannelAndId extends MidiControlWithChannel {
	// @minimum 0
	// @maximum 127
	id: number,
}

export interface MidiNote extends MidiControlWithChannelAndId {
	type: MidiControlType.NOTE_ON | MidiControlType.NOTE_OFF | MidiControlType.NOTE,
}

export interface MidiControlChange extends MidiControlWithChannelAndId {
	type: MidiControlType.CONTROL_CHANGE,
}

export interface MidiProgramChange extends MidiControlWithChannelAndId {
	type: MidiControlType.PROGRAM_CHANGE,
}

export interface MidiPitchBend extends MidiControlWithChannel {
	type: MidiControlType.PITCH_BEND,
}

export interface MidiChannelPressure extends MidiControlWithChannel {
	type: MidiControlType.CHANNEL_PRESSURE,
}

export interface MidiPolyAftertouch extends MidiControlWithChannelAndId {
	type: MidiControlType.POLY_AFTERTOUCH,
}

export interface MidiStart extends MidiControlBase {
	type: MidiControlType.START,
}

export interface MidiContinue extends MidiControlBase {
	type: MidiControlType.CONTINUE,
}

export interface MidiStop extends MidiControlBase {
	type: MidiControlType.STOP,
}

export interface MidiReset extends MidiControlBase {
	type: MidiControlType.RESET,
}

export type MidiControl =
	MidiNote | MidiControlChange | MidiProgramChange | MidiPitchBend |
	MidiChannelPressure | MidiPolyAftertouch | MidiStart |
	MidiContinue | MidiStop | MidiReset;

export type PortName = string;

export interface midi_schema_v1 {
	[port: string] : {
		//output_to?: PortName | Array<PortName>,
		//input_from?: PortName | Array<PortName>,
		controls: {
			[key: string]: MidiControl
		}
	}
};

export type midi_schema = midi_schema_v1;
