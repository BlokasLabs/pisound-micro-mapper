#ifndef PISOUND_MICRO_CONTROL_MANAGER_H
#define PISOUND_MICRO_CONTROL_MANAGER_H

#include <vector>
#include <map>
#include <set>
#include <memory>
#include <cstring>

#include "control-server.h"

class ControlManager : protected IControlServer::IListener
{
public:
	ControlManager();

	void addControlServer(std::shared_ptr<IControlServer> server);

	struct map_options_t
	{
		int m_src_ch;
		int m_dst_ch;
	};

	static map_options_t defaultMapOptions();

	void map(IControl &src, IControl &dst, const map_options_t &opts = defaultMapOptions());

	int subscribe();

	size_t getNumFds() const;
	int fillFds(struct pollfd *fds, size_t n) const;
	int handleFdEvents(struct pollfd *fds, size_t nfds, size_t nevents);

private:
	typedef uint32_t milliseconds_t;
	static milliseconds_t now();

	void maskControlChangeEvent(milliseconds_t ts, IControl *control, IControl::value_t value, int ch);
	bool isControlChangeEventMasked(IControl *control, IControl::value_t value, int ch) const;
	void unmaskControlChangeEvent(IControl *control, IControl::value_t value, int ch);

	virtual void onControlChange(IControl *control, int ch) override;

	struct server_t
	{
		std::shared_ptr<IControlServer> m_server;
		mutable size_t m_numFds;
	};

	struct mapping_t
	{
		IControl      *m_src;
		IControl      *m_dst;
		int           m_src_ch;
		int           m_dst_ch;

		inline bool operator<(const mapping_t &rhs) const
		{
			return memcmp(this, &rhs, sizeof(*this)) < 0;
		}
	};

	void processTimeouts(milliseconds_t now);

	struct masked_control_change_event_t
	{
		milliseconds_t    m_ts;
		IControl          *m_control;
		IControl::value_t m_value;
		int               m_ch;

		//inline bool operator==(const masked_control_change_event_t &rhs) const
		//{
		//	//return m_ch < 0 || rhs.m_ch < 0 ? m_control == rhs.m_control && memcmp(&m_value, &rhs.m_value, sizeof(m_value)) == 0 : memcmp(this, &rhs, sizeof(*this)) == 0;
		//	return memcmp(this, &rhs, sizeof(*this)) == 0;
		//}
	};

	std::vector<server_t> m_ctrlServers;
	std::map<IControl*, std::set<mapping_t> > m_mappings;

	class MaskedControlChangeEventPredicate;

	std::vector<masked_control_change_event_t> m_maskedControlChangeEvents;
};

#endif // PISOUND_MICRO_CONTROL_MANAGER_H
