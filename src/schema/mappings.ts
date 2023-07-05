export enum MappingDirection {
	A_TO_B = "->",
	B_TO_A = "<-",
	BOTH = "<->",
};
export type Mapping = [string, MappingDirection, string, object? ];
