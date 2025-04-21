// SPDX-License-Identifier: GPL-2.0-only
//
// pisound-micro-mapper - a daemon to facilitate Pisound Micro control mappings.
//  Copyright (C) 2023-2025  Vilniaus Blokas UAB, https://blokas.io/
//
// This file is part of pisound-micro-mapper.
//
// pisound-micro-mapper is free software: you can redistribute it and/or modify it under the terms of the
// GNU General Public License as published by the Free Software Foundation, version 2.0 of the License.
//
// pisound-micro-mapper is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even
// the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
// for more details.
//
// You should have received a copy of the GNU General Public License along with pisound-micro-mapper.
// If not, see <https://www.gnu.org/licenses/>.

import { alsa_schema_v1 } from "./alsa_schema";
import { midi_schema_v1 } from "./midi_schema";
import { osc_schema_v1 } from "./osc_schema";
import { pisound_micro_schema_v1 } from "./pisound_micro_schema";
import { config_schema_v1 } from "./config_schema";

export interface pisound_micro_root_v1 extends config_schema_v1 {
	controls: {
		alsa?: alsa_schema_v1;
		midi?: midi_schema_v1;
		osc?: osc_schema_v1;
		"pisound-micro"?: pisound_micro_schema_v1;
		[other: string]: unknown;
	};
}
export type pisound_micro_root = pisound_micro_root_v1;
