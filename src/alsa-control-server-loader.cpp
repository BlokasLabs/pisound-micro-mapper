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

#include "alsa-control-server-loader.h"

#include "alsa-control-server.h"
#include "logger.h"
#include "control-manager.h"

#include <errno.h>

#include <rapidjson/document.h>

extern "C" const char *get_alsa_schema();
extern "C" size_t get_alsa_schema_length();

struct alsa_control_info_t
{
	std::string name;
	std::string alias;
};

static bool parseControlInfo(alsa_control_info_t &info, const rapidjson::Value &value)
{
	if (value.IsString())
	{
		info.name = info.alias = value.GetString();
		return true;
	}
	else if (value.IsArray() && value.Size() == 2 && value[1].IsObject())
	{
		auto alias = value[1].FindMember("alias");
		if (alias->value.IsString())
		{
			info.name = value[0].GetString();
			info.alias = alias->value.GetString();
			return true;
		}
	}
	return false;
}

const char *AlsaControlServerLoader::getJsonName() const
{
	return "alsa";
}

int AlsaControlServerLoader::sanitizeJson(rapidjson::Value &object, rapidjson::Document::AllocatorType &allocator) const
{
	return 0;
}

int AlsaControlServerLoader::verifyJson(const rapidjson::Value &object) const
{
	return ConfigLoader::verifyJson(get_alsa_schema(), get_alsa_schema_length(), object);
}

int AlsaControlServerLoader::processJson(ControlManager &mgr, IControlRegister &reg, const rapidjson::Value &object)
{
	int err;
	alsa_control_info_t ctrlInfo;

	for (auto card = object.MemberBegin(); card != object.MemberEnd(); ++card)
	{
		auto name = card->name.GetString();
		std::shared_ptr<AlsaControlServer> alsa = std::make_shared<AlsaControlServer>();

		if ((err = alsa->init(name)) < 0)
		{
			LOG_ERROR(R"(Failed to init AlsaControlServer for "%s"! (%d))", name, err);
			return err;
		}

		for (auto ctrl = card->value.Begin(); ctrl != card->value.End(); ++ctrl)
		{
			if (!parseControlInfo(ctrlInfo, *ctrl))
			{
				LOG_ERROR(R"(Failed parsing AlsaControlServer "%s" controls!)", name);
				return -EPROTO;
			}

			IControl *ctl = alsa->registerControl(ctrlInfo.name.c_str());
			if (!ctl)
			{
				LOG_ERROR(R"(Control "%s" not found in AlsaControlServer "%s"!)", ctrlInfo.name.c_str(), name);
				return -ENOENT;
			}
			reg.registerControl(ctrlInfo.alias, *ctl);
		}

		mgr.addControlServer(alsa);
	}

	return 0;
}
