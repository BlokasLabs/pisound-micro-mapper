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

#ifndef PISOUND_MICRO_CONFIG_LOADER_H
#define PISOUND_MICRO_CONFIG_LOADER_H

#include <map>
#include <string>

#include <rapidjson/document.h>

class IControl;
class ControlManager;

class IControlRegister
{
public:
	virtual bool registerControl(std::string name, IControl &ctrl) = 0;
	virtual IControl *get(const std::string &name) const = 0;
};

class IControlServerLoader
{
public:
	virtual ~IControlServerLoader() = 0;

	virtual const char *getJsonName() const = 0;
	virtual int sanitizeJson(rapidjson::Value &object, rapidjson::Document::AllocatorType &allocator) const = 0;
	virtual int verifyJson(const rapidjson::Value &object) const = 0;
	virtual int processJson(ControlManager &mgr, IControlRegister &reg, const rapidjson::Value &object) = 0;
};

class ConfigLoader
{
public:
	ConfigLoader();

	void registerControlServerLoader(IControlServerLoader &loader);

	int processJson(ControlManager &mgr, rapidjson::Document &config);

	static int verifyJson(const char *schema, size_t len, const rapidjson::Value &object);

private:
	std::map<std::string, IControlServerLoader*> m_csLoaders;
};

#endif // PISOUND_MICRO_CONFIG_LOADER_H
