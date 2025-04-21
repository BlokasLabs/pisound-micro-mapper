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

// Standalone appplication to test JSON config file against schema.
#include <rapidjson/document.h>
#include <rapidjson/schema.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/writer.h>

#include <fstream>
#include <iostream>

int main(int argc, char **argv)
{
	if (argc < 3)
	{
		std::cerr << "Usage: " << argv[0] << " <schema.json> <config.json>" << std::endl;
		return 1;
	}

	std::ifstream schemaStream(argv[1], std::ios::in);
	if (!schemaStream.is_open())
	{
		std::cerr << "Failed to open " << argv[1] << std::endl;
		return 1;
	}

	rapidjson::Document d;

	rapidjson::IStreamWrapper ifw(schemaStream);
	if (d.ParseStream(ifw).HasParseError())
	{
		std::cerr << "Failed to parse " << argv[1] << std::endl;
		return 1;
	}

	rapidjson::SchemaDocument schemaDoc(d);
	rapidjson::SchemaValidator validator(schemaDoc);

	std::ifstream jsonStream(argv[2], std::ios::in);

	if (!jsonStream.is_open())
	{
		std::cerr << "Failed to open " << argv[2] << std::endl;
		return 1;
	}

	rapidjson::IStreamWrapper jfw(jsonStream);
	if (d.ParseStream(jfw).HasParseError())
	{
		std::cerr << "Failed to parse " << argv[2] << std::endl;
		return 1;
	}

	if (!d.Accept(validator))
	{
		std::cerr << "Failed to validate " << argv[2] << std::endl;
		rapidjson::StringBuffer sb;
		validator.GetInvalidSchemaPointer().StringifyUriFragment(sb);
		std::cerr << "Invalid schema: " << sb.GetString() << std::endl;
		std::cerr << "Invalid keyword: " << validator.GetInvalidSchemaKeyword() << std::endl;
		sb.Clear();
		validator.GetInvalidDocumentPointer().StringifyUriFragment(sb);
		std::cerr << "Invalid document: " << sb.GetString() << std::endl;
		sb.Clear();
		rapidjson::Writer<rapidjson::StringBuffer> w(sb);
		validator.GetError().Accept(w);
		std::cerr << "Error message: " << sb.GetString() << std::endl;

		return 1;
	}

	std::cout << argv[2] << " is valid" << std::endl;

	return 0;
}
