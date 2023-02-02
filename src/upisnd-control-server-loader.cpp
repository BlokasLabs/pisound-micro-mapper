#include "upisnd-control-server-loader.h"

const char *PisoundMicroControlServerLoader::getJsonName() const
{
	return "pisound-micro";
}

int PisoundMicroControlServerLoader::processJson(ControlManager &mgr, IControlRegister &reg, const rapidjson::Value::ConstObject &object)
{
	return 0;
}
