export enum MappingDirection {
	A_TO_B = "->",
	B_TO_A = "<-",
	BOTH = "<->",
};
export type mapping_v1 = [string, MappingDirection, string, object? ];
