import { Mapping } from "./mappings";

export interface config_schema {
	version: number;
	controls: object;
	mappings?: Array<Mapping>;
}
