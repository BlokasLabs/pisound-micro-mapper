export type AlsaControl = string | [ string, { alias?: string }? ];

export interface Root {
	[card: string] : Array<AlsaControl>
};
