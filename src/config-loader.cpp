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

#include "control-manager.h"
#include "config-loader.h"

#include "logger.h"

#include <errno.h>

#include <rapidjson/document.h>
#include <rapidjson/schema.h>
#include <rapidjson/error/en.h>

extern "C" const char *get_config_schema(void);
extern "C" size_t get_config_schema_length(void);

enum MappingDir
{
	UNKNOWN = 0,
	A_TO_B = 1 << 0,
	B_TO_A = 1 << 1,
	BOTH   = A_TO_B | B_TO_A
};

struct mapping_info_t
{
	std::string                   a;
	MappingDir                    dir;
	std::string                   b;
	ControlManager::map_options_t opts;
};

ConfigLoader::ConfigLoader()
{
}

void ConfigLoader::registerControlServerLoader(IControlServerLoader &loader)
{
	m_csLoaders[loader.getJsonName()] = &loader;
}

class ControlRegister : public IControlRegister
{
public:
	virtual bool registerControl(std::string name, IControl &ctrl) override
	{
		auto ret = m_reg.insert(std::make_pair(std::move(name), &ctrl));
		if (!ret.second)
		{
			LOG_ERROR(R"(Control "%s" already existed, use an alias or rename!)", ret.first->first.c_str());
			return false;
		}

		return true;
	}

	virtual IControl *get(const std::string &name) const override
	{
		auto item = m_reg.find(name);
		return item != m_reg.end() ? item->second : NULL;
	}

private:
	std::map<std::string, IControl*> m_reg;
};

static MappingDir parseDir(const std::string &s)
{
	if (s == "->")
		return A_TO_B;
	if (s == "<-")
		return B_TO_A;
	if (s == "<->")
		return BOTH;
	return UNKNOWN;
}

static int parseMappingOptions(ControlManager::map_options_t &opts, rapidjson::Value::ConstObject o)
{
	opts = ControlManager::defaultMapOptions();
	auto ch = o.FindMember("src_ch");
	if (ch != o.MemberEnd())
	{
		opts.m_src_ch = ch->value.GetInt();
	}
	ch = o.FindMember("dst_ch");
	if (ch != o.MemberEnd())
	{
		opts.m_dst_ch = ch->value.GetInt();
	}
	return 0;
}

static int parseMapping(mapping_info_t &info, rapidjson::Value::ConstArray m)
{
	if (m.Size() < 3 || m.Size() > 4)
		return -EINVAL;

	info.a = m[0].GetString();
	info.dir = parseDir(m[1].GetString());
	if (info.dir == UNKNOWN)
		return -EINVAL;
	info.b = m[2].GetString();

	if (m.Size() == 4)
	{
		return parseMappingOptions(info.opts, m[3].GetObject());
	}
	else info.opts = ControlManager::defaultMapOptions();

	return 0;
}

int ConfigLoader::verifyJson(const char *schema, size_t len, const rapidjson::Value &object)
{
	rapidjson::Document d;
	d.Parse(schema, len);
	if (d.HasParseError())
	{
		LOG_ERROR(R"(Internal error, failed to parse built-in config schema: %s (%u))", rapidjson::GetParseError_En(d.GetParseError()), d.GetErrorOffset());
		return -EHWPOISON;
	}

	rapidjson::SchemaDocument schemaDoc(d);
	rapidjson::SchemaValidator validator(schemaDoc);

	if (!object.Accept(validator))
	{
		rapidjson::StringBuffer sb;
		validator.GetInvalidSchemaPointer().StringifyUriFragment(sb);
		LOG_ERROR(R"(Invalid config entry: %s)", sb.GetString());
		return -EPROTO;
	}
	return 0;
}

int ConfigLoader::processJson(ControlManager &mgr, rapidjson::Document &config)
{
	if (!config.IsObject())
		return -EINVAL;

	int err = verifyJson(get_config_schema(), get_config_schema_length(), config.GetObject());
	if (err < 0)
		return err;

	ControlRegister reg;
	const rapidjson::Value *value = NULL;

	int controlIdx = 1;

	auto item = config.FindMember("version");
	if (item != config.MemberEnd() && item->value.GetInt() != 1)
		return -EPROTO;

	item = config.FindMember("controls");
	for (auto ctrl = item->value.MemberBegin(); ctrl != item->value.MemberEnd(); ++ctrl)
	{
		auto srv = ctrl->name.GetString();
		decltype(m_csLoaders)::const_iterator loader = m_csLoaders.find(srv);
		if (loader == m_csLoaders.end())
		{
			LOG_ERROR(R"(No loader for "%s")", srv);
			continue;
		}

		err = loader->second->sanitizeJson(ctrl->value.GetObject(), config.GetAllocator());
		if (err < 0)
		{
			LOG_ERROR(R"(Sanitizing "%s" resulted in error %d!)", srv, err);
			goto error;
		}

		err = loader->second->verifyJson(ctrl->value.GetObject());
		if (err < 0)
		{
			LOG_ERROR(R"(Verifying "%s" resulted in error %d!)", srv, err);
			goto error;
		}

		err = loader->second->processJson(mgr, reg, ctrl->value.GetObject());

		if (err < 0)
		{
			LOG_ERROR(R"(Processing "%s" resulted in error %d!)", srv, err);
			goto error;
		}
	}

	item = config.FindMember("mappings");
	value = &item->value;
	for (auto m = value->Begin(); m != value->End(); ++m)
	{
		mapping_info_t info;
		err = parseMapping(info, m->GetArray());
		if (err < 0)
		{
			LOG_ERROR(R"(Parsing mapping %d failed with error %d!)", controlIdx, err);
			goto error;
		}

		IControl *c[2] = { reg.get(info.a), reg.get(info.b) };
		for (int i=0; i<2; ++i)
		{
			if (!c[i])
			{
				LOG_ERROR(R"(Mapped control %d "%s" not found!)", controlIdx, (i == 0 ? info.a : info.b).c_str());
				err = -ENOENT;
				goto error;
			}
		}

		if (info.dir & A_TO_B)
			mgr.map(*c[0], *c[1], info.opts);
		if (info.dir & B_TO_A)
		{
			std::swap(info.opts.m_src_ch, info.opts.m_dst_ch);
			mgr.map(*c[1], *c[0], info.opts);
		}

		++controlIdx;
	}

error:
	if (err < 0)
	{
		// restore to empty state.
	}
	return err;
}
