import { alsa_schema_v1 } from "./alsa_schema";
import { midi_schema_v1 } from "./midi_schema";
import { osc_schema_v1 } from "./osc_schema";
import { pisound_micro_schema_v1 } from "./pisound_micro_schema";
import { config_schema_v1 } from "./config_schema";

export interface pisound_micro_root_v1 extends config_schema_v1 {
	controls: {
		alsa?: alsa_schema_v1;
		midi?: midi_schema_v1;
		osc?: osc_schema_v1;
		"pisound-micro"?: pisound_micro_schema_v1;
		[other: string]: unknown;
	};
}
export type pisound_micro_root = pisound_micro_root_v1;
