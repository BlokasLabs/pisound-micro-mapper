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
