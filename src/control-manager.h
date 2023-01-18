#ifndef PISOUND_MICRO_CONTROL_MANAGER_H
#define PISOUND_MICRO_CONTROL_MANAGER_H

#include <vector>
#include <map>

#include "control-server.h"

class ControlManager : protected IControlServer::IListener
{
public:
	ControlManager();

	void addControlServer(IControlServer *server);

	void map(IControl &from, IControl &to);

	size_t getNumFds() const;
	int fillFds(struct pollfd *fds, size_t n) const;
	int handleFdEvents(struct pollfd *fds, size_t nfds, size_t nevents);

private:
	virtual void onControlChange(IControl *control) override;

	struct server_t
	{
		IControlServer* m_server;
		mutable size_t m_numFds;
	};

	std::vector<server_t> m_ctrlServers;
	std::multimap<IControl*, IControl*> m_mappings;
};

#endif // PISOUND_MICRO_CONTROL_MANAGER_H
