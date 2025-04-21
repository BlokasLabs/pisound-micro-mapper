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

#include "osc-control-server-loader.h"
#include "osc-control-server.h"

#include "control-manager.h"
#include "logger.h"

#include <lo/lo_cpp.h>
#include <cstdint>
#include <string>

extern "C" const char *get_osc_schema();
extern "C" size_t get_osc_schema_length();

struct osc_control_info_t
{
	std::string path;
	OscType     type;
	osc_value_t low;
	osc_value_t high;
};

static OscType str_to_osc_type(const char *s)
{
	if (strncmp(s, "f", 2) == 0) return OSC_TYPE_FLOAT;
	if (strncmp(s, "i", 2) == 0) return OSC_TYPE_INT;
	return OSC_TYPE_UNKNOWN;
}

static int parseValue(osc_value_t &value, OscType type, const rapidjson::Value &val)
{
	switch (type)
	{
	case OSC_TYPE_FLOAT:
		value.f = val.GetFloat();
		break;
	case OSC_TYPE_INT:
		value.i = val.GetInt();
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static int parseControlInfo(osc_control_info_t &info, const char *name, const rapidjson::Value &ctrl)
{
	int err;
	info.path = ctrl["path"].GetString();
	info.type = str_to_osc_type(ctrl["type"].GetString());
	if (info.type == OSC_TYPE_UNKNOWN)
	{
		LOG_ERROR(R"(Failed to parse type for "%s"!)", name);
		return -EINVAL;
	}
	auto low = ctrl.FindMember("low");
	if (low != ctrl.MemberEnd())
	{
		if ((err = parseValue(info.low, info.type, low->value)) != 0)
		{
			LOG_ERROR(R"(Failed to parse low value for "%s"! (%d))", name, err);
			return err;
		}
	}
	else if (info.type == OSC_TYPE_FLOAT)
	{
		info.low.f = 0.0f;
	}
	else
	{
		info.low.i = 0;
	}

	auto high = ctrl.FindMember("high");
	if (high != ctrl.MemberEnd())
	{
		if ((err = parseValue(info.high, info.type, high->value)) != 0)
		{
			LOG_ERROR(R"(Failed to parse high value for "%s"! (%d))", name, err);
			return err;
		}
	}
	else if (info.type == OSC_TYPE_FLOAT)
	{
		info.high.f = 1.0f;
	}
	else
	{
		info.high.i = 1023;
	}
	return 0;
}

const char *OscControlServerLoader::getJsonName() const
{
	return "osc";
}

int OscControlServerLoader::sanitizeJson(rapidjson::Value &object, rapidjson::Document::AllocatorType &allocator) const
{
	return 0;
}

int OscControlServerLoader::verifyJson(const rapidjson::Value &object) const
{
	return ConfigLoader::verifyJson(get_osc_schema(), get_osc_schema_length(), object);
}

int OscControlServerLoader::processJson(ControlManager &mgr, IControlRegister &reg, const rapidjson::Value &object)
{
	int err;
	for (auto srvObj = object.MemberBegin(); srvObj != object.MemberEnd(); ++srvObj)
	{
		std::shared_ptr<OscControlServer> srv = std::make_shared<OscControlServer>();

		const char *addr = NULL;
		auto listen = srvObj->value.FindMember("listen");
		if (listen != srvObj->value.MemberEnd())
		{
			addr = listen->value.GetString();
		}

		if ((err = srv->init(srvObj->name.GetString(), addr)) != 0)
		{
			LOG_ERROR(R"(Failed to init OscControlServer for "%s"! (%d))", srvObj->name.GetString(), err);
			return err;
		}

		auto nfy = srvObj->value.FindMember("notify");
		if (nfy != srvObj->value.MemberEnd())
		{
			if (nfy->value.IsString())
			{
				srv->addNotify(nfy->value.GetString());
			}
			else if (nfy->value.IsArray())
			{
				for (auto nfyAddr = nfy->value.Begin(); nfyAddr != nfy->value.End(); ++nfyAddr)
				{
					srv->addNotify(nfyAddr->GetString());
				}
			}
			else
			{
				LOG_ERROR(R"(Failed to parse notify object for "%s"!)", srvObj->name.GetString());
				return -EINVAL;
			}
		}

		auto params = srvObj->value.FindMember("params");
		for (auto param = params->value.MemberBegin(); param != params->value.MemberEnd(); ++param)
		{
			osc_control_info_t info;
			if ((err = parseControlInfo(info, param->name.GetString(), param->value)) != 0)
			{
				LOG_ERROR(R"(Failed to parse control info for "%s"! (%d))", param->name.GetString(), err);
				return err;
			}
			IControl *c;
			if (info.type == OSC_TYPE_FLOAT)
				c = srv->registerFloatControl(param->name.GetString(), info.path.c_str(), info.low.f, info.high.f);
			else c = srv->registerIntControl(param->name.GetString(), info.path.c_str(), info.low.i, info.high.i);

			reg.registerControl(param->name.GetString(), *c);
		}

		mgr.addControlServer(srv);
	}

	return 0;
}
