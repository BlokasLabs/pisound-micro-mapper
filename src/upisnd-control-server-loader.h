#ifndef PISOUND_MICRO_CONTROL_SERVER_LOADER_H
#define PISOUND_MICRO_CONTROL_SERVER_LOADER_H

#include "config-loader.h"

class PisoundMicroControlServerLoader : public IControlServerLoader
{
public:
	virtual const char *getJsonName() const override;
	virtual int processJson(ControlManager &mgr, IControlRegister &reg, const rapidjson::Value::ConstObject &object) override;

private:
};

#endif // PISOUND_MICRO_CONTROL_SERVER_LOADER_H
