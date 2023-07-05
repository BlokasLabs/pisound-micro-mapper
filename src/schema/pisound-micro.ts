// Control types will be transformed to lower case in json data prior to validation.
export enum ControlType {
	ENCODER = "encoder",
	ANALOG_IN = "analog_in",
	GPIO_IN = "gpio_in",
	GPIO_OUT = "gpio_out",
	ACTIVITY_LED = "activity_led",
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

export interface ControlBase
{
	type: ControlType;
}

export interface ControlValueBase extends ControlBase {
	input_low?: number;
	input_high?: number;
	value_low?: number;
	value_high?: number;
}

export enum EncoderValueMode {
	CLAMP = "clamp",
	WRAP = "wrap",
};

export interface Encoder extends ControlValueBase {
	type: ControlType.ENCODER;
	pins: [ PinName, PinPull, PinName, PinPull ];
	mode?: EncoderValueMode;
}

export interface AnalogInput extends ControlValueBase {
	type: ControlType.ANALOG_IN;
	pin: PinName;
}

export interface GpioInput {
	type: ControlType.GPIO_IN;
	pin: [ PinName, PinPull ];
}

export interface GpioOutput {
	type: ControlType.GPIO_OUT;
	pin: PinName;

	/**
	 * @minimum 0
	 * @maximum 1
	 */
	value: boolean | number;
}

// RapidJSON uses draft-04 schema validation, which misses entries with incorrect "type" field,
// this will be checked manually by traversing the json data before validating it.
export type Control = Encoder | AnalogInput | GpioInput | GpioOutput;
