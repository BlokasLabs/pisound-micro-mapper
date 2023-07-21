#ifndef PISOUND_MICRO_OSC_CONTROL_SERVER_LOADER_H
#define PISOUND_MICRO_OSC_CONTROL_SERVER_LOADER_H

#include "config-loader.h"

class OscControlServerLoader : public IControlServerLoader
{
public:
	virtual const char *getJsonName() const override;
	virtual int sanitizeJson(rapidjson::Value &object, rapidjson::Document::AllocatorType &allocator) const override;
	virtual int verifyJson(const rapidjson::Value &object) const override;
	virtual int processJson(ControlManager &mgr, IControlRegister &reg, const rapidjson::Value &object) override;
};

#endif // PISOUND_MICRO_OSC_CONTROL_SERVER_LOADER_H
