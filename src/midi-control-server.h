#ifndef PISOUND_MICRO_MIDI_CONTROL_SERVER_H
#define PISOUND_MICRO_MIDI_CONTROL_SERVER_H

#include "control-server.h"

#include <alsa/asoundlib.h>

#include <map>

enum MidiControlType
{
	MCT_UNKNOWN=-1,
	MCT_NOTE_ON,
	MCT_NOTE_OFF,
	MCT_CONTROL_CHANGE,
	MCT_PITCH_BEND,
	MCT_PROGRAM_CHANGE,
	MCT_CHANNEL_PRESSURE,
	MCT_POLY_AFTERTOUCH,
	MCT_START,
	MCT_CONTINUE,
	MCT_STOP,
	MCT_RESET,
};

class MidiControlServer : public IControlServer
{
public:
	MidiControlServer();
	virtual ~MidiControlServer();

	int init(const char *name);

	virtual void setListener(IListener *listener) override;

	virtual int subscribe() override;

	virtual size_t getNumFds() const override;
	virtual int fillFds(struct pollfd *fds, size_t n) const override;
	virtual int handleFdEvents(struct pollfd *fds, size_t nfds, size_t nevents) override;

	IControl *registerControl(const char *name, MidiControlType type, int8_t channel, int8_t id);

private:
	void handleEvent(const snd_seq_event_t *ev);
	int sendEvent(MidiControlType type, int8_t channel, int8_t id, int16_t value);

	class Control;
	typedef int16_t control_id_t;

	static control_id_t buildId(MidiControlType type, int8_t channel, int8_t id);
	static control_id_t identify(int16_t &value, const snd_seq_event_t *ev);

	void uninit();

	IListener *m_listener;
	snd_seq_t *m_seq;
	int       m_port;

	std::multimap<control_id_t, Control> m_controls;
};

#endif // PISOUND_MICRO_MIDI_CONTROL_SERVER_H
