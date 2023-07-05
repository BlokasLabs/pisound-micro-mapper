#include "upisnd-control-server-loader.h"
#include "upisnd-control-server.h"

#include "control-manager.h"

#include "logger.h"

#include <errno.h>

const char *PisoundMicroControlServerLoader::getJsonName() const
{
	return "pisound-micro";
}

static void parse_value_range(upisnd_range_t &range, const rapidjson::Value &v, const char *low, const char *high)
{
	auto m = v.FindMember(low);
	if (m != v.MemberEnd())
		range.low = m->value.GetInt();
	m = v.FindMember(high);
	if (m != v.MemberEnd())
		range.high = m->value.GetInt();
}

enum ElementType
{
	UNKNOWN,
	ENCODER,
	ANALOG_INPUT,
	GPIO_INPUT,
	GPIO_OUTPUT,
	ACTIVITY_LED,
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
	case UPISND_ELEMENT_TYPE_ACTIVITY_LED:
		return ACTIVITY_LED;
	default:
		return UNKNOWN;
	}
}

int PisoundMicroControlServerLoader::processJson(ControlManager &mgr, IControlRegister &reg, const rapidjson::Value::ConstObject &object)
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
				auto v = ctrl->value["pins"].GetArray();
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
				parse_value_range(opts.input_range, ctrl->value, "input_low", "input_high");
				parse_value_range(opts.value_range, ctrl->value, "value_low", "value_high");
				auto m = ctrl->value.FindMember("mode");
				if (m != ctrl->value.MemberEnd())
					opts.value_mode = upisnd_str_to_value_mode(ctrl->value["mode"].GetString());
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
				parse_value_range(opts.input_range, ctrl->value, "input_low", "input_high");
				parse_value_range(opts.value_range, ctrl->value, "value_low", "value_high");
				in.setOpts(opts);

				el = in;
			}
			break;
		case GPIO_INPUT:
			{
				auto v = ctrl->value["pin"].GetArray();
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
				auto v = ctrl->value.FindMember("value");
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
		case ACTIVITY_LED:
			LOG_INFO("Activity LED is not supported yet, skipping...");
			continue;
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
