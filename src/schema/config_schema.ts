import { Mapping } from "./mappings";

export interface Root {
	version: number;
	controls: object;
	mappings?: Array<Mapping>;
}
