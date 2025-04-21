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

#include "upisnd-control-server-loader.h"
#include "upisnd-control-server.h"

#include "control-manager.h"

#include "logger.h"

#include <ctype.h>
#include <errno.h>
#include <pisound-micro.h>

#include <rapidjson/document.h>

extern "C" const char *get_pisound_micro_schema();
extern "C" size_t get_pisound_micro_schema_length();

const char *PisoundMicroControlServerLoader::getJsonName() const
{
	return "pisound-micro";
}

static bool parse_value_range(upisnd_range_t &range, const rapidjson::Value &v, const char *low, const char *high)
{
	bool found = false;
	auto m = v.FindMember(low);
	if (m != v.MemberEnd())
	{
		range.low = m->value.GetInt();
		found = true;
	}
	m = v.FindMember(high);
	if (m != v.MemberEnd())
	{
		range.high = m->value.GetInt();
		found = true;
	}
	return found;
}

enum ElementType
{
	UNKNOWN,
	ENCODER,
	ANALOG_INPUT,
	GPIO_INPUT,
	GPIO_OUTPUT,
	ACTIVITY,
};

static ElementType str_to_element_type(const char *s)
{
	if (strncmp(s, "gpio_input", 11) == 0)
		return GPIO_INPUT;
	else if (strncmp(s, "gpio_output", 12) == 0)
		return GPIO_OUTPUT;

	upisnd_element_type_e t = upisnd_str_to_element_type(s);
	switch (t)
	{
	case UPISND_ELEMENT_TYPE_ENCODER:
		return ENCODER;
	case UPISND_ELEMENT_TYPE_ANALOG_INPUT:
		return ANALOG_INPUT;
	case UPISND_ELEMENT_TYPE_ACTIVITY:
		return ACTIVITY;
	default:
		return UNKNOWN;
	}
}

static std::string to_lower(const char *str)
{
	std::string ret;
	while (*str)
		ret += tolower(*str++);
	return ret;
}

static int sanitizePins(rapidjson::Value &pins, rapidjson::Document::AllocatorType &allocator)
{
	if (pins.IsString())
	{
		pins.SetString(upisnd_pin_to_str(upisnd_str_to_pin(pins.GetString())), allocator);
		return 0;
	}

	if (pins.IsArray())
	{
		int i=0;
		for (auto item = pins.Begin(); item != pins.End(); ++item, ++i)
		{
			if (!item->IsString())
				return -EINVAL;

			if ((i & 1) == 0)
				item->SetString(upisnd_pin_to_str(upisnd_str_to_pin(item->GetString())), allocator);
			else
				item->SetString(to_lower(item->GetString()).c_str(), allocator);
		}

		return 0;
	}

	return -EINVAL;
}

int PisoundMicroControlServerLoader::sanitizeJson(rapidjson::Value &object, rapidjson::Document::AllocatorType &allocator) const
{
	if (!object.IsObject())
		return -EINVAL;

	std::vector<std::string> to_remove;
	int err = 0;

	for (auto ctrl = object.MemberBegin(); ctrl != object.MemberEnd(); ++ctrl)
	{
		auto pins = ctrl->value.FindMember("pins");
		if (pins != ctrl->value.MemberEnd())
		{
			if ((err = sanitizePins(pins->value, allocator)) < 0)
				return err;
		}
		pins = ctrl->value.FindMember("pin");
		if (pins != ctrl->value.MemberEnd())
		{
			if ((err = sanitizePins(pins->value, allocator)) < 0)
				return err;
		}
		auto type = ctrl->value.FindMember("type");
		if (type != ctrl->value.MemberEnd())
		{
			if (type->value.IsString())
			{
				type->value.SetString(to_lower(type->value.GetString()).c_str(), allocator);
			}
			if (!type->value.IsString() || str_to_element_type(type->value.GetString()) == UNKNOWN)
			{
				fprintf(stderr, "Unknown control type '%s', ignoring control '%s'...\n", type->value.GetString(), ctrl->name.GetString());
				to_remove.push_back(ctrl->name.GetString());
			}
			else
			{
				switch (str_to_element_type(type->value.GetString()))
				{
				case ACTIVITY:
					{
						auto activity = ctrl->value.FindMember("activity");
						if (activity != ctrl->value.MemberEnd() && activity->value.IsString())
						{
							activity->value.SetString(to_lower(activity->value.GetString()).c_str(), allocator);
							if (upisnd_str_to_activity(activity->value.GetString()) == UPISND_ACTIVITY_INVALID)
							{
								fprintf(stderr, "Unknown activity '%s', ignoring control '%s'...\n", activity->value.GetString(), ctrl->name.GetString());
								to_remove.push_back(ctrl->name.GetString());
							}
						}
					}
					break;
				default:
					break;
				}
			}
		}
	}
	for (const auto &name : to_remove)
		object.RemoveMember(name.c_str());
	return 0;
}

int PisoundMicroControlServerLoader::verifyJson(const rapidjson::Value &object) const
{
	return ConfigLoader::verifyJson(get_pisound_micro_schema(), get_pisound_micro_schema_length(), object);
}

int PisoundMicroControlServerLoader::processJson(ControlManager &mgr, IControlRegister &reg, const rapidjson::Value &object)
{
	if (object.MemberCount() == 0)
		return 0;

	std::shared_ptr<PisoundMicroControlServer> server = std::make_shared<PisoundMicroControlServer>();

	for (auto ctrl = object.MemberBegin(); ctrl != object.MemberEnd(); ++ctrl)
	{
		ElementType type = str_to_element_type(ctrl->value["type"].GetString());
		upisnd::Element el;
		switch (type)
		{
		case ENCODER:
			{
				const auto &v = ctrl->value["pins"].GetArray();
				auto enc = upisnd::Encoder::setup(
					ctrl->name.GetString(),
					upisnd_str_to_pin(v[0].GetString()),
					upisnd_str_to_pin_pull(v[1].GetString()),
					upisnd_str_to_pin(v[2].GetString()),
					upisnd_str_to_pin_pull(v[3].GetString())
				);

				if (!enc.isValid())
				{
					LOG_ERROR("Failed to setup encoder '%s'", ctrl->name.GetString());
					continue;
				}

				upisnd_encoder_opts_t opts;
				upisnd_element_encoder_init_default_opts(&opts);
				bool inputRangeFound = parse_value_range(opts.input_range, ctrl->value, "input_min", "input_max");
				bool valueRangeFound = parse_value_range(opts.value_range, ctrl->value, "value_low", "value_high");
				const auto &m = ctrl->value.FindMember("mode");
				if (m != ctrl->value.MemberEnd())
					opts.value_mode = upisnd_str_to_value_mode(ctrl->value["mode"].GetString());

				// If only one range is specified, use it for both.
				if (inputRangeFound ^ valueRangeFound)
				{
					if (inputRangeFound) opts.value_range = opts.input_range;
					else opts.input_range = opts.value_range;
				}

				enc.setOpts(opts);
				el = enc;
			}
			break;
		case ANALOG_INPUT:
			{
				auto in = upisnd::AnalogInput::setup(
					ctrl->name.GetString(),
					upisnd_str_to_pin(ctrl->value["pin"].GetString())
				);

				if (!in.isValid())
				{
					LOG_ERROR("Failed to setup analog input '%s'", ctrl->name.GetString());
					continue;
				}

				upisnd_analog_input_opts_t opts;
				upisnd_element_analog_input_init_default_opts(&opts);
				bool inputRangeFound = parse_value_range(opts.input_range, ctrl->value, "input_min", "input_max");
				bool valueRangeFound = parse_value_range(opts.value_range, ctrl->value, "value_low", "value_high");

				// If only one range is specified, use it for both.
				if (inputRangeFound ^ valueRangeFound)
				{
					if (inputRangeFound) opts.value_range = opts.input_range;
					else opts.input_range = opts.value_range;
				}

				in.setOpts(opts);

				el = in;
			}
			break;
		case GPIO_INPUT:
			{
				const auto &v = ctrl->value["pin"].GetArray();
				auto gpio = upisnd::Gpio::setupInput(ctrl->name.GetString(), upisnd_str_to_pin(v[0].GetString()), upisnd_str_to_pin_pull(v[1].GetString()));
				if (!gpio.isValid())
				{
					LOG_ERROR("Failed to setup GPIO input '%s'", ctrl->name.GetString());
					continue;
				}
				el = gpio;
			}
			break;
		case GPIO_OUTPUT:
			{
				bool high = true;
				const auto &v = ctrl->value.FindMember("value");
				if (v->value.IsBool())
					high = v->value.GetBool();
				else if (v->value.IsNumber())
					high = v->value.GetInt() != 0;
				auto gpio = upisnd::Gpio::setupOutput(ctrl->name.GetString(), upisnd_str_to_pin(ctrl->value["pin"].GetString()), high);
				if (!gpio.isValid())
				{
					LOG_ERROR("Failed to setup GPIO output '%s'", ctrl->name.GetString());
					continue;
				}
				el = gpio;
			}
			break;
		case ACTIVITY:
			{
				auto activity = upisnd::Activity::setupActivity(
					ctrl->name.GetString(),
					upisnd_str_to_pin(ctrl->value["pin"].GetString()),
					upisnd_str_to_activity(ctrl->value["activity"].GetString())
					);
				if (!activity.isValid())
				{
					LOG_ERROR("Failed to setup Activity '%s'", ctrl->name.GetString());
					continue;
				}
				el = activity;
			}
			break;
		default:
			LOG_INFO("Unknown control type '%s', skipping...", ctrl->value["type"].GetString());
			continue;
		}

		if (el.isValid())
		{
			IControl *c = server->registerControl(el);
			if (!c)
			{
				LOG_ERROR("Failed to register control '%s'", ctrl->name.GetString());
				continue;
			}

			reg.registerControl(ctrl->name.GetString(), *c);
		}
	}

	mgr.addControlServer(server);

	return 0;
}
