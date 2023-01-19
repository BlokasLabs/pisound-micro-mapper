#include "control-manager.h"

#include <cassert>
#include <poll.h>

ControlManager::ControlManager()
{
}

void ControlManager::addControlServer(IControlServer *server)
{
	m_ctrlServers.push_back({ server, 0 });
	server->setListener(this);
}

void ControlManager::map(IControl &from, IControl &to)
{
	m_mappings.insert(std::make_pair(&from, &to));
}

size_t ControlManager::getNumFds() const
{
	size_t n=0;
	for (const auto &itr : m_ctrlServers)
		n += itr.m_server->getNumFds();
	return n;
}

int ControlManager::fillFds(struct pollfd *fds, size_t n) const
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

int ControlManager::handleFdEvents(struct pollfd *fds, size_t nfds, size_t nevents)
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

static int calc(int value, int src_low, int src_high, int dst_low, int dst_high)
{
	return (int)(((float)(value - src_low) / (src_high - src_low)) * (dst_high - dst_low) + dst_low);
}

void ControlManager::onControlChange(IControl *from)
{
	printf("Control %s changed! Value: %d\n", from->getName(), from->getValue(-1));

	int value = from->getValue(-1);
	int src_low = from->getLow();
	int src_high = from->getHigh();

	auto items = m_mappings.equal_range(from);
	for (auto itr = items.first; itr != items.second; ++itr)
	{
		IControl *to = itr->second;
		int dst_low = to->getLow();
		int dst_high = to->getHigh();

		int v = calc(value, src_low, src_high, dst_low, dst_high);
		printf("v=%d (%d, %d, %d, %d, %d) -> %d\n", v, value, src_low, src_high, dst_low, dst_high, to->setValue(v, 0));

		//for (int idx=0; idx<to->getMemberCount(); ++idx)
		//	to->setValue(v, idx);
	}
}	
