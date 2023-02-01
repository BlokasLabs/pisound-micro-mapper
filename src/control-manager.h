#ifndef PISOUND_MICRO_CONTROL_MANAGER_H
#define PISOUND_MICRO_CONTROL_MANAGER_H

#include <vector>
#include <map>
#include <memory>

#include "control-server.h"

class ControlManager : protected IControlServer::IListener
{
public:
	ControlManager();

	void addControlServer(std::shared_ptr<IControlServer> server);

	struct map_options_t
	{
		int m_index;
	};

	static map_options_t defaultMapOptions();

	void map(IControl &from, IControl &to, const map_options_t &opts = defaultMapOptions());

	int subscribe();

	size_t getNumFds() const;
	int fillFds(struct pollfd *fds, size_t n) const;
	int handleFdEvents(struct pollfd *fds, size_t nfds, size_t nevents);

private:
	void maskControlChangeEvent(IControl *control, int value, int index);
	bool isControlChangeEventMasked(IControl *control, int value, int index) const;
	void unmaskControlChangeEvent(IControl *control);

	virtual void onControlChange(IControl *control) override;

	struct server_t
	{
		std::shared_ptr<IControlServer> m_server;
		mutable size_t m_numFds;
	};

	struct mapping_t
	{
		IControl      *m_from;
		IControl      *m_to;
		map_options_t m_opts;
	};

	struct masked_control_change_event_t
	{
		IControl *m_control;
		int      m_value;
		int      m_index;

		inline bool operator==(const masked_control_change_event_t &rhs) const
		{
			return m_control == rhs.m_control && m_value == rhs.m_value && m_index == rhs.m_index;
		}
	};

	std::vector<server_t> m_ctrlServers;
	std::multimap<IControl*, mapping_t> m_mappings;
	std::vector<masked_control_change_event_t> m_maskedControlChangeEvents;
};

#endif // PISOUND_MICRO_CONTROL_MANAGER_H
