import { mapping_v1 } from "./mappings";

export interface schema_base {
	$schema: "https://blokas.io/json/pisound-micro-schema.json";
	version: number;
}

export interface config_schema_v1 extends schema_base {
	version: 1;
	controls: object;
	mappings?: Array<mapping_v1>;
}

export type config_schema = config_schema_v1;
