export type AlsaControl = string | [ string, { alias?: string }? ];

export interface alsa_schema_v1 {
	[card: string] : Array<AlsaControl>
};

export type alsa_schema = alsa_schema_v1;
