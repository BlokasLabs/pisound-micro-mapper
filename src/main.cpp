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

#include <cassert>
#include <cstdio>
#include <poll.h>
#include <signal.h>
#include <unistd.h>

#include <vector>
#include <fstream>

#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/error/error.h>
#include <rapidjson/error/en.h>

#include "control-manager.h"
#include "control-server.h"
#include "config-loader.h"
#include "alsa-control-server-loader.h"
#include "midi-control-server-loader.h"
#include "upisnd-control-server-loader.h"
#include "osc-control-server-loader.h"
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
		return -ENOENT;
	}

	ConfigLoader cfgLoader;

	AlsaControlServerLoader alsaLoader;
	PisoundMicroControlServerLoader upisndLoader;
	MidiControlServerLoader midiLoader;
	OscControlServerLoader oscLoader;

	cfgLoader.registerControlServerLoader(alsaLoader);
	cfgLoader.registerControlServerLoader(upisndLoader);
	cfgLoader.registerControlServerLoader(midiLoader);
	cfgLoader.registerControlServerLoader(oscLoader);

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

static void printVersion()
{
	printf("pisound-micro-mapper version 1.0.0 © Blokas https://blokas.io/\n");
}

static void printUsage()
{
	printf(
R"(pisound-micro-mapper usage:

	pisound-micro-mapper [--config <config.json>]

	--config <config.json>      Load the config from the specified file. Default: /etc/pisound-micro-mapper.json
	--help                      Print this help
	--version                   Print version

	See https://blokas.io/pisound-micro/docs/pisound-micro-mapper/ for more information.

)");
	printVersion();
}

int main(int argc, char **argv)
{
	const char *config_file = "/etc/pisound-micro-mapper.json";

	for (int i = 1; i < argc; ++i)
	{
		if (strncmp(argv[i], "--config", 9) == 0)
		{
			if (i + 1 >= argc)
			{
				printUsage();
				return EXIT_FAILURE;
			}

			config_file = argv[i + 1];
			++i;
		}
		else if (strcmp(argv[i], "--help") == 0)
		{
			printUsage();
			return EXIT_SUCCESS;
		}
		else if (strcmp(argv[i], "--version") == 0)
		{
			printVersion();
			return EXIT_SUCCESS;
		}
		else
		{
			printUsage();
			return EXIT_FAILURE;
		}
	}

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

	ControlManager mgr;

	int err = loadConfig(mgr, config_file);
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
