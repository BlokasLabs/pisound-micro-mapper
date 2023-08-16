export enum Type {
	Float = 'f',
	Integer = 'i',
}

export interface Param {
	type: Type;
	path: string;
	low?: number;
	high?: number;
}

export type Address = string;

export interface osc_schema_v1 {
	[ name: string ]: {
		listen?: Address;
		notify?: Address | Array<Address>;
		params: {
			[ param: string ]: Param;
 		}
	}
}

export type osc_schema = osc_schema_v1;
