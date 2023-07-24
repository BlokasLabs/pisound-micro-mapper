import { Mapping } from "./mappings";
import { alsa_schema } from "./alsa_schema";
import { midi_schema } from "./midi_schema";
import { osc_schema } from "./osc_schema";
import { pisound_micro_schema } from "./pisound_micro_schema";

export interface SchemaRoot {
	version: number;
	controls: {
		alsa?: alsa_schema;
		midi?: midi_schema;
		osc?: osc_schema;
		"pisound-micro"?: pisound_micro_schema;
	};
	mappings?: Array<Mapping>;
}
