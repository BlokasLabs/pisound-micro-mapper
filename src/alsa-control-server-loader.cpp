#include "alsa-control-server-loader.h"

#include "alsa-control-server.h"
#include "logger.h"
#include "control-manager.h"

#include <errno.h>

struct control_info_t
{
	std::string name;
	std::string alias;
};

static bool parseControlInfo(control_info_t &info, const rapidjson::Value &value)
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

int AlsaControlServerLoader::processJson(ControlManager &mgr, IControlRegister &reg, const rapidjson::Value::ConstObject &object)
{
	int err;
	control_info_t ctrlInfo;

	// Validate schema...

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
