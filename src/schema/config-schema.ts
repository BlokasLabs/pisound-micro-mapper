import { Control } from "./pisound-micro";
import { AlsaControl } from "./alsa";
import { Mapping } from "./mappings";

export interface ConfigRoot {
	version: number;
	controls: {
		"pisound-micro"?: { [element: string]: Control },
		alsa?: { [card: string] : Array<AlsaControl> }
	};
	mappings?: Array<Mapping>;
}
