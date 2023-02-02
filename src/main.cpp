#include <cassert>
#include <cstdio>
#include <poll.h>
#include <signal.h>

#include <vector>
#include <fstream>

#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/error/error.h>
#include <rapidjson/error/en.h>

#include "control-manager.h"
#include "control-server.h"
#include "alsa-control-server.h"
#include "upisnd-control-server.h"
#include "config-loader.h"
#include "alsa-control-server-loader.h"
#include "upisnd-control-server-loader.h"
#include "logger.h"

static volatile sig_atomic_t signal_received = 0;

static void signal_handler(int s)
{
	signal_received = s;
}

static int mainloop(ControlManager &mgr)
{
	sigset_t emptyset;
	sigemptyset(&emptyset);
	int err;
	std::vector<pollfd> pfds;

	err = mgr.subscribe();
	if (err < 0)
		return err;

	while (signal_received == 0)
	{
		pfds.resize(mgr.getNumFds());
		if ((err = mgr.fillFds(pfds.data(), pfds.size())) < 0)
			return err;

		err = ppoll(pfds.data(), pfds.size(), NULL, &emptyset);

		if (signal_received)
			break;

		if (err < 0)
			return err;

		if (err == 0)
			continue;

		mgr.handleFdEvents(pfds.data(), pfds.size(), err);
	}

	return 0;
}

static int loadConfig(ControlManager &mgr, const char *file)
{
	std::ifstream stream;

	stream.open(file, std::ifstream::in);

	if (!stream.good())
	{
		LOG_ERROR(R"(Failed to open "%s"!)", file);
	}

	ConfigLoader cfgLoader;

	AlsaControlServerLoader alsaLoader;
	PisoundMicroControlServerLoader upisndLoader;

	cfgLoader.registerControlServerLoader(alsaLoader);
	cfgLoader.registerControlServerLoader(upisndLoader);

	rapidjson::Document doc;
	rapidjson::IStreamWrapper isw(stream);
	rapidjson::ParseResult ok = doc.ParseStream(isw);

	if (!ok)
	{
		LOG_ERROR(R"(Parsing "%s" failed: %s (%u))", file, rapidjson::GetParseError_En(ok.Code()), ok.Offset());
		return -EPROTO;
	}

	return cfgLoader.processJson(mgr, doc);
}

int main(int argc, char **argv)
{
	static StdioLogger stdioLogger;
	Logger::registerLogger(stdioLogger);
	Logger::setEnabled(true);

	struct sigaction sa;
	sigemptyset(&sa.sa_mask);
	sigaddset(&sa.sa_mask, SIGINT);
	sigaddset(&sa.sa_mask, SIGTERM);
	pthread_sigmask(SIG_BLOCK, &sa.sa_mask, NULL);

	sa.sa_handler = signal_handler;
	sa.sa_flags = 0;

	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);

#if 0
	int err;
	upisnd::LibInitializer initializer;

	if (initializer.getResult() < 0)
	{
		LOG_ERROR("Failed to initialize libpisoundmicro! Error %d (%m)", errno);
		return 1;
	}

	std::shared_ptr<AlsaControlServer> alsa = std::make_shared<AlsaControlServer>();

	if ((err = alsa->init("hw:micro")) < 0)
	{
		LOG_ERROR("Failed to init AlsaControlServer! (%d)", err);
		return 1;
	}

	std::shared_ptr<PisoundMicroControlServer> pmcs = std::make_shared<PisoundMicroControlServer>();
#endif

	ControlManager mgr;

	int err = loadConfig(mgr, "example.json");
	if (err < 0)
	{
		LOG_ERROR("Loading config failed with error %d! (%s)", err, strerror(-err));
		return EXIT_FAILURE;
	}

	LOG_INFO("pid: %d", getpid());

	err = mainloop(mgr);
	if (err < 0)
	{
		LOG_ERROR("Main loop exited with error %d!", err);
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
