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

// Control types will be transformed to lower case in json data prior to validation.
export enum ControlType {
	/**
	 * An Encoder control. Required properties: `pins`, optional properties: `input_min`, `input_max`, `value_low`, `value_high`, `mode`.
	 */
	ENCODER = "encoder",
	/**
	 * An Analog Input control. Required properties: `pin`, optional properties: `input_min`, `input_max`, `value_low`, `value_high`.
	 */
	ANALOG_IN = "analog_in",
	/**
	 * A GPIO Input control. Required property: `pin`.
	 */
	GPIO_IN = "gpio_input",
	/**
	 * A GPIO Output control. Required properties: `pin`, `value`.
	 */
	GPIO_OUT = "gpio_output",
	/**
	 * An Activity control. Required properties: `activity`, `pin`.
	 */
	ACTIVITY = "activity",
}

// The pin names will be transformed to upper case in json data prior to validation.
export enum PinName {
	A27 = "A27", A28 = "A28", A29 = "A29", A30 = "A30", A31 = "A31", A32 = "A32", B03 = "B03", B04 = "B04",
	B05 = "B05", B06 = "B06", B07 = "B07", B08 = "B08", B09 = "B09", B10 = "B10", B11 = "B11", B12 = "B12",
	B13 = "B13", B14 = "B14", B15 = "B15", B16 = "B16", B17 = "B17", B18 = "B18", B23 = "B23", B24 = "B24",
	B25 = "B25", B26 = "B26", B27 = "B27", B28 = "B28", B29 = "B29", B30 = "B30", B31 = "B31", B32 = "B32",
	B33 = "B33", B34 = "B34", B37 = "B37", B38 = "B38", B39 = "B39"
}

// The pull values will be transformed to lower case in json data prior to validation.
export enum PinPull {
	PULL_NONE = "pull_none",
	PULL_UP   = "pull_up",
	PULL_DOWN = "pull_down",
}

export enum ActivityType {
	MIDI_IN = "midi_in",
	MIDI_OUT = "midi_out",
}

export interface ControlBase
{
	type: ControlType;
}

export interface ControlValueBase extends ControlBase {
	input_min?: number;
	input_max?: number;
	value_low?: number;
	value_high?: number;
}

export enum EncoderValueMode {
	/**
	 * The Encoder's value will stay within the boundaries.
	 */
	CLAMP = "clamp",

	/**
	 * The Encoder's value will wrap around to the other boundary.
	 */
	WRAP = "wrap",
};

export interface Encoder extends ControlValueBase {
	type: ControlType.ENCODER;
	/**
	 * [ Pin A, Pin A Pull, Pin B, Pin B Pull ]
	 */
	pins: [ PinName, PinPull, PinName, PinPull ];
	mode?: EncoderValueMode;
}

export interface AnalogInput extends ControlValueBase {
	type: ControlType.ANALOG_IN;
	pin: PinName;
}

export interface GpioInput extends ControlBase {
	type: ControlType.GPIO_IN;
	/**
	 * [ Pin, Pull ]
	 */
	pin: [ PinName, PinPull ];
}

export interface GpioOutput extends ControlBase {
	type: ControlType.GPIO_OUT;
	pin: PinName;

	/**
	 * Either a boolean true/false or 0 or 1.
	 *
	 * @minimum 0
	 * @maximum 1
	 */
	value: boolean | number;
}

export interface Activity extends ControlBase {
	type: ControlType.ACTIVITY;
	activity: ActivityType;
	pin: PinName;
}

// RapidJSON uses draft-04 schema validation, which misses entries with incorrect "type" field,
// this will be checked manually by traversing the json data before validating it.
export type Control = Activity | Encoder | AnalogInput | GpioInput | GpioOutput;

export interface pisound_micro_schema_v1 {
	[element: string]: Control
}

export type pisound_micro_schema = pisound_micro_schema_v1;
