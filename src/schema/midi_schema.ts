export enum MidiControlType {
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
	channel?: number,
}

export interface MidiNoteOn extends MidiControlWithChannel {
	type: MidiControlType.NOTE_ON,
	// @minimum 0
	// @maximum 127
	id: number,
}

export interface MidiNoteOff extends MidiControlWithChannel {
	type: MidiControlType.NOTE_OFF,
	// @minimum 0
	// @maximum 127
	id: number,
}

export interface MidiControlChange extends MidiControlWithChannel {
	type: MidiControlType.CONTROL_CHANGE,
	// @minimum 0
	// @maximum 127
	id: number,
}

export interface MidiProgramChange extends MidiControlWithChannel {
	type: MidiControlType.PROGRAM_CHANGE,
	// @minimum 0
	// @maximum 127
	id: number,
}

export interface MidiPitchBend extends MidiControlWithChannel {
	type: MidiControlType.PITCH_BEND,
}

export interface MidiChannelPressure extends MidiControlWithChannel {
	type: MidiControlType.CHANNEL_PRESSURE,
}

export interface MidiPolyAftertouch extends MidiControlWithChannel {
	type: MidiControlType.POLY_AFTERTOUCH,
	// @minimum 0
	// @maximum 127
	id: number,
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
	MidiNoteOn | MidiNoteOff | MidiControlChange | MidiProgramChange |
	MidiPitchBend | MidiChannelPressure | MidiPolyAftertouch | MidiStart |
	MidiContinue | MidiStop | MidiReset;

export type PortName = string;

export interface midi_schema {
	[port: string] : {
		output_to?: PortName | Array<PortName>,
		input_from?: PortName | Array<PortName>,
		controls: {
			[key: string]: MidiControl
		}
	}
};
