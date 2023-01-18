#include <pisound-micro.h>
#include <alsa/asoundlib.h>
#include <cassert>
#include <cstdio>
#include <poll.h>
#include <signal.h>

#include <vector>
#include <map>

#include "control-server.h"
#include "alsa-control-server.h"
#include "upisnd-control-server.h"

class ControlManager : protected IControlServer::IListener
{
public:
	ControlManager()
	{
	}

	void addControlServer(IControlServer *server)
	{
		m_ctrlServers.push_back({ server, 0 });
		server->setListener(this);
	}

	size_t getNumFds() const
	{
		size_t n=0;
		for (const auto &itr : m_ctrlServers)
			n += itr.m_server->getNumFds();
		return n;
	}

	int fillFds(struct pollfd *fds, size_t n) const
	{
		size_t total = 0;
		for (const auto &itr : m_ctrlServers)
		{
			int cnt = itr.m_server->fillFds(fds, n);
			if (cnt < 0)
			{
				fprintf(stderr, "IControlServer::fillFds failed! (%d)\n", cnt);
				return cnt;
			}
			assert(cnt <= n);

			total += cnt;
			n -= cnt;
			fds += cnt;

			itr.m_numFds = cnt;
		}
		return total;
	}

	int handleFdEvents(struct pollfd *fds, size_t nfds, size_t nevents)
	{
		size_t total;
		for (auto &itr : m_ctrlServers)
		{
			int n = itr.m_server->handleFdEvents(fds, itr.m_numFds, nevents);
			fds += itr.m_numFds;

			if (n < 0)
			{
				fprintf(stderr, "IControlServer::handleFdEvents failed! (%d)\n", n);
				return n;
			}

			assert(n <= nevents);

			total += n;

			nevents -= n;

			if (nevents == 0)
				break;
		}

		return total;
	}

private:
	virtual void onControlChange(IControl *control) override
	{
		printf("Control %s changed! Value: %d\n", control->getName(), control->getValue(-1));
	}

	struct server_t
	{
		IControlServer* m_server;
		mutable size_t m_numFds;
	};

	std::vector<server_t> m_ctrlServers;
};

class MappingManager
{
public:


private:
	std::multimap<IControl*, IControl*> m_mappings;
};

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
	while (signal_received == 0)
	{
		pfds.resize(mgr.getNumFds());
		if ((err = mgr.fillFds(pfds.data(), pfds.size())) < 0)
		{
			return err;
		}

		err = ppoll(pfds.data(), pfds.size(), NULL, &emptyset);

		if (signal_received)
		{
			printf("Signal received! S %d errno %d (%m)\n", signal_received, errno);
			break;
		}

		if (err < 0)
			return err;

		if (err == 0)
			continue;

		mgr.handleFdEvents(pfds.data(), pfds.size(), err);
	}

	return 0;
}

int main(int argc, char **argv)
{
	struct sigaction sa;
	sigemptyset(&sa.sa_mask);
	sigaddset(&sa.sa_mask, SIGINT);
	sigaddset(&sa.sa_mask, SIGTERM);
	pthread_sigmask(SIG_BLOCK, &sa.sa_mask, NULL);

	sa.sa_handler = signal_handler;
	sa.sa_flags = 0;

	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);

	int err;
	upisnd::LibInitializer initializer;

	if (initializer.getResult() < 0)
	{
		fprintf(stderr, "Failed to initialize libpisoundmicro! Error %d (%m)\n", errno);
		return 1;
	}

#if 0
	snd_ctl_t *ctl;
	int err = snd_ctl_open(&ctl, "hw:micro", SND_CTL_NONBLOCK);

	if (err != 0)
		return 1;

	err = snd_ctl_subscribe_events(ctl, 1);
	printf("Subs: %d\n", err);

	snd_ctl_card_info_t *info;
	snd_ctl_card_info_alloca(&info);

	snd_ctl_card_info(ctl, info);

	const char *name = snd_ctl_card_info_get_name(info);

	snd_ctl_elem_id_t *id;
	snd_ctl_elem_value_t *value;
	snd_ctl_elem_id_alloca(&id);
	snd_ctl_elem_value_alloca(&value);

	snd_ctl_elem_id_set_interface(id, SND_CTL_ELEM_IFACE_MIXER);
	snd_ctl_elem_id_set_name(id, "Digital Playback Volume");
	if (err = lookup_id(id, ctl))
		return err;

	snd_ctl_elem_value_set_id(value, id);
	snd_ctl_elem_value_set_integer(value, 0, 55);
	snd_ctl_elem_value_set_integer(value, 1, 77);

	if ((err = snd_ctl_elem_write(ctl, value)) < 0) {
		fprintf(stderr, "Control element write error: %s\n",
			snd_strerror(err));
		return err;
	}

	err = snd_ctl_poll_descriptors_count(ctl);
	printf("Descs: %d\n", err);

	struct pollfd pfd[1];
	memset(pfd, 0, sizeof(pfd));
	err = snd_ctl_poll_descriptors(ctl, pfd, 1);
	printf("snd_ctl_poll_descriptors: %d\n", err);

	printf("fd=%d events=%08x revents=%08x\n", pfd[0].fd, pfd[0].events, pfd[0].revents);

	err = poll(pfd, 1, 10000);
	printf("poll = %d\n", err);

	printf("fd=%d events=%08x revents=%08x\n", pfd[0].fd, pfd[0].events, pfd[0].revents);

	//printf("Waiting...\n");
	////err = snd_ctl_wait(ctl, 10000);
	//printf("Done %d\n", err);

	snd_ctl_event_t *event;
	snd_ctl_event_alloca(&event);
	do
	{
		err = snd_ctl_read(ctl, event);
		printf("Read: %d\n", err);

		printf("Type: %d\n", snd_ctl_event_get_type(event));
		printf("Numid: %08x\n", snd_ctl_event_elem_get_numid(event));
	}
	while (err > 0);

	snd_ctl_close(ctl);
#endif

	AlsaControlServer alsa;

	if ((err = alsa.init("hw:micro")) < 0)
	{
		fprintf(stderr, "Failed to init AlsaControlServer (%d)\n", err);
		return 1;
	}

	alsa.registerControl("Digital Playback Volume");
	alsa.registerControl("Digital Capture Volume");

	PisoundMicroControlServer pmcs;

	upisnd::Encoder enc = upisnd::Encoder::setup("encoder", UPISND_PIN_B03, UPISND_PIN_PULL_UP, UPISND_PIN_B04, UPISND_PIN_PULL_UP);

	pmcs.registerControl(enc);

	ControlManager mgr;
	mgr.addControlServer(&alsa);
	mgr.addControlServer(&pmcs);

	printf("pid: %d\n", getpid());

	return mainloop(mgr);
}
