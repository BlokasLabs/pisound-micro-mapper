#ifndef PISOUND_MICRO_CONFIG_LOADER_H
#define PISOUND_MICRO_CONFIG_LOADER_H

#include <map>
#include <string>

#include <rapidjson/document.h>

class IControl;
class ControlManager;

class IControlRegister
{
public:
	virtual bool registerControl(std::string name, IControl &ctrl) = 0;
	virtual IControl *get(const std::string &name) const = 0;
};

class IControlServerLoader
{
public:
	virtual ~IControlServerLoader() = 0;

	virtual const char *getJsonName() const = 0;
	virtual int sanitizeJson(rapidjson::Value &object, rapidjson::Document::AllocatorType &allocator) const = 0;
	virtual int verifyJson(const rapidjson::Value &object) const = 0;
	virtual int processJson(ControlManager &mgr, IControlRegister &reg, const rapidjson::Value &object) = 0;
};

class ConfigLoader
{
public:
	ConfigLoader();

	void registerControlServerLoader(IControlServerLoader &loader);

	int processJson(ControlManager &mgr, rapidjson::Document &config);

	static int verifyJson(const char *schema, size_t len, const rapidjson::Value &object);

private:
	std::map<std::string, IControlServerLoader*> m_csLoaders;
};

#endif // PISOUND_MICRO_CONFIG_LOADER_H
