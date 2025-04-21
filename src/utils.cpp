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

#include "utils.h"

std::string to_std_string(IControl::value_t value, IControl::Type t)
{
	std::string str;
	switch (t)
	{
	case IControl::Type::INT:
		str = std::to_string(value.i);
		break;
	case IControl::Type::FLOAT:
		str = std::to_string(value.f);
		break;
	default:
		str = "(unrecognized type)";
		break;
	}
	return str;
}
