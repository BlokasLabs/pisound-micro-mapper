#include "midi-control-server-loader.h"

#include "midi-control-server.h"
#include "logger.h"
#include "control-manager.h"

#include <errno.h>

#include <rapidjson/document.h>

extern "C" const char *get_midi_schema();
extern "C" size_t get_midi_schema_length();

struct midi_control_info_t
{
	MidiControlType type;
	int8_t channel;
	int8_t id;
};

static MidiControlType str_to_mct(const char *s)
{
	if (0 == strncmp("note", s, 5))              return MCT_NOTE;
	if (0 == strncmp("note_on", s, 8))           return MCT_NOTE_ON;
	if (0 == strncmp("note_off", s, 9))          return MCT_NOTE_OFF;
	if (0 == strncmp("control_change", s, 15))   return MCT_CONTROL_CHANGE;
	if (0 == strncmp("program_change", s, 15))   return MCT_PROGRAM_CHANGE;
	if (0 == strncmp("pitch_bend", s, 11))       return MCT_PITCH_BEND;
	if (0 == strncmp("channel_pressure", s, 17)) return MCT_CHANNEL_PRESSURE;
	if (0 == strncmp("poly_aftertouch", s, 16))  return MCT_POLY_AFTERTOUCH;
	if (0 == strncmp("start", s, 6))             return MCT_START;
	if (0 == strncmp("stop", s, 5))              return MCT_STOP;
	if (0 == strncmp("continue", s, 9))          return MCT_CONTINUE;
	if (0 == strncmp("reset", s, 6))             return MCT_RESET;

	return MCT_UNKNOWN;
}

static bool parseControlInfo(midi_control_info_t &info, const rapidjson::Value &value)
{
	if (!value.IsObject())
		return false;

	auto type = value.FindMember("type");
	if (type == value.MemberEnd() || !type->value.IsString())
		return false;

	info.type = str_to_mct(type->value.GetString());
	if (info.type == MCT_UNKNOWN)
		return false;

	auto channel = value.FindMember("channel");
	if (channel == value.MemberEnd())
		info.channel = -1;
	else if (!channel->value.IsInt())
		return false;
	else
	{
		int c = channel->value.GetInt();
		if (c < 1 || c > 16)
			return false;
		info.channel = c-1;
	}

	switch (info.type)
	{
	case MCT_NOTE:
	case MCT_NOTE_ON:
	case MCT_NOTE_OFF:
	case MCT_CONTROL_CHANGE:
	case MCT_PROGRAM_CHANGE:
	case MCT_POLY_AFTERTOUCH:
		{
			auto id = value.FindMember("id");
			if (id == value.MemberEnd() || !id->value.IsInt())
				return false;
			int i = id->value.GetInt();
			if (i < 0 || i > 127)
				return false;
			info.id = i;
		}
		break;
	default:
		info.id = -1;
		break;
	}

	return true;
}

const char *MidiControlServerLoader::getJsonName() const
{
	return "midi";
}

int MidiControlServerLoader::sanitizeJson(rapidjson::Value &object, rapidjson::Document::AllocatorType &allocator) const
{
	return 0;
}

int MidiControlServerLoader::verifyJson(const rapidjson::Value &object) const
{
	return ConfigLoader::verifyJson(get_midi_schema(), get_midi_schema_length(), object);
}

int MidiControlServerLoader::processJson(ControlManager &mgr, IControlRegister &reg, const rapidjson::Value &object)
{
	int err;
	midi_control_info_t ctrlInfo;

	for (auto port = object.MemberBegin(); port != object.MemberEnd(); ++port)
	{
		auto name = port->name.GetString();
		std::shared_ptr<MidiControlServer> midi = std::make_shared<MidiControlServer>();

		if ((err = midi->init(name)) < 0)
		{
			LOG_ERROR(R"(Failed to init MidiControlServer for "%s"! (%d))", name, err);
			return err;
		}

		auto controls = port->value.FindMember("controls");
		if (controls == port->value.MemberEnd() || !controls->value.IsObject())
		{
			LOG_ERROR(R"(Failed to find "controls" object in MidiControlServer config!)");
			return -EPROTO;
		}

		for (auto ctrl = controls->value.MemberBegin(); ctrl != controls->value.MemberEnd(); ++ctrl)
		{
			if (!parseControlInfo(ctrlInfo, ctrl->value))
			{
				LOG_ERROR(R"(Failed parsing MidiControlServer "%s" controls!)", name);
				return -EPROTO;
			}

			IControl *ctl = midi->registerControl(ctrl->name.GetString(), ctrlInfo.type, ctrlInfo.channel, ctrlInfo.id);
			if (!ctl)
			{
				LOG_ERROR(R"(Control "%s" could not be registered with MidiControlServer "%s"!)", ctrl->name.GetString(), name);
				return -ENOENT;
			}
			reg.registerControl(ctrl->name.GetString(), *ctl);
		}

		mgr.addControlServer(midi);
	}

	return 0;
}
