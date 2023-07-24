export type AlsaControl = string | [ string, { alias?: string }? ];

export interface alsa_schema {
	[card: string] : Array<AlsaControl>
};
