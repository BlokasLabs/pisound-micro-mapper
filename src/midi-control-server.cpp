#include "midi-control-server.h"
#include "logger.h"

// based on https://github.com/BlokasLabs/amidithru/blob/master/amidithru.cpp

MidiControlServer::control_id_t MidiControlServer::buildId(MidiControlType type, int8_t channel, int8_t id)
{
	return ((type&0xf) << 11) | ((channel&0xf) << 7) | (id&0x7f);
}

MidiControlServer::control_id_t MidiControlServer::identify(int16_t &value, const snd_seq_event_t *ev)
{
	MidiControlType type;
	int8_t channel;
	int8_t id;

	switch (ev->type)
	{
	case SND_SEQ_EVENT_NOTEON:
		type = MCT_NOTE_ON;
		channel = ev->data.note.channel;
		id = ev->data.note.note;
		value = ev->data.note.velocity;
		break;
	case SND_SEQ_EVENT_NOTEOFF:
		type = MCT_NOTE_OFF;
		channel = ev->data.note.channel;
		id = ev->data.note.note;
		value = ev->data.note.velocity;
		break;
	case SND_SEQ_EVENT_KEYPRESS:
		type = MCT_POLY_AFTERTOUCH;
		channel = ev->data.note.channel;
		id = ev->data.note.note;
		value = ev->data.note.velocity;
		break;
	case SND_SEQ_EVENT_CONTROLLER:
		type = MCT_CONTROL_CHANGE;
		channel = ev->data.control.channel;
		id = ev->data.control.param;
		value = ev->data.control.value;
		break;
	case SND_SEQ_EVENT_PGMCHANGE:
		type = MCT_PROGRAM_CHANGE;
		channel = ev->data.control.channel;
		id = ev->data.control.value;
		value = 0;
		break;
	case SND_SEQ_EVENT_CHANPRESS:
		type = MCT_CHANNEL_PRESSURE;
		channel = ev->data.control.channel;
		id = 0;
		value = ev->data.control.value;
		break;
	case SND_SEQ_EVENT_PITCHBEND:
		type = MCT_PITCH_BEND;
		channel = ev->data.control.channel;
		id = 0;
		value = ev->data.control.value;
		break;
	case SND_SEQ_EVENT_START:
		type = MCT_START;
		channel = 0;
		id = 0;
		value = 0;
		break;
	case SND_SEQ_EVENT_CONTINUE:
		type = MCT_CONTINUE;
		channel = 0;
		id = 0;
		value = 0;
		break;
	case SND_SEQ_EVENT_STOP:
		type = MCT_STOP;
		channel = 0;
		id = 0;
		value = 0;
		break;
	case SND_SEQ_EVENT_RESET:
		type = MCT_RESET;
		channel = 0;
		id = 0;
		value = 0;
		break;
	default:
		value = -1;
		return -1;
	}

	return buildId(type, channel, id);
}

class MidiControlServer::Control : public IControl
{
public:
	Control(MidiControlServer &srv, const char *name, MidiControlType type, int8_t channel, int8_t id)
		:m_srv(srv)
		,m_name(strdup(name))
		,m_type(type)
		,m_channel(channel)
		,m_id(id)
	{
	}

	~Control()
	{
		free((char*)m_name);
	}

	const char *getName() const override
	{
		return m_name;
	}

	virtual value_t getLow() const override
	{
		return { .i = m_type != MCT_PITCH_BEND ? 0 : -8192 };
	}

	virtual value_t getHigh() const override
	{
		return { .i = m_type != MCT_PITCH_BEND ? 127 : 8191 };
	}

	virtual Type getType() const override
	{
		return INT;
	}

	virtual int getMemberCount() const override
	{
		return 1;
	}

	void updateValue(int16_t value)
	{
		m_value = value;
	}

	virtual int setValue(value_t value, int index) override
	{
		m_value = value.i;
		return m_srv.sendEvent(m_type, m_channel, m_id, value.i);
	}

	virtual value_t getValue(int index) const override
	{
		return { .i = m_value };
	}

private:
	MidiControlServer &m_srv;
	const char        *m_name;
	MidiControlType   m_type;
	int8_t            m_channel;
	int8_t            m_id;
	int16_t           m_value;
};

MidiControlServer::MidiControlServer()
	:m_listener(NULL)
	,m_seq(NULL)
	,m_port(0)
{
}

MidiControlServer::~MidiControlServer()
{
	uninit();
}

int MidiControlServer::init(const char *name)
{
	if (m_seq)
	{
		LOG_ERROR("Already initialized!\n");
		return -EINVAL;
	}

	int err = snd_seq_open(&m_seq, "default", SND_SEQ_OPEN_DUPLEX, 0);
	if (err < 0) {
		LOG_ERROR("Error opening ALSA sequencer. (%d)\n", err);
		return err;
	}

	err = snd_seq_set_client_name(m_seq, name);
	if (err < 0) {
		LOG_ERROR("Error setting client name. (%d)\n", err);
		goto error;
	}

	err = snd_seq_create_simple_port(
		m_seq,
		name,
		SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE |
		SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_SUBS_READ |
		SND_SEQ_PORT_CAP_DUPLEX,
		SND_SEQ_PORT_TYPE_HARDWARE | SND_SEQ_PORT_TYPE_MIDI_GENERIC
		);

	if (err < 0)
	{
		LOG_ERROR("Couldn't create a virtual MIDI port! (%d)\n", err);
		goto error;
	}

	m_port = err;

	return 0;

error:
	if (m_port)
	{
		snd_seq_delete_simple_port(m_seq, m_port);
		m_port = 0;
	}
	if (m_seq)
	{
		snd_seq_close(m_seq);
		m_seq = NULL;
	}
	return err;
}

void MidiControlServer::uninit()
{
	if (m_port)
	{
		snd_seq_delete_simple_port(m_seq, m_port);
		m_port = 0;
	}
	if (m_seq)
	{
		snd_seq_close(m_seq);
		m_seq = NULL;
	}
}

void MidiControlServer::setListener(IListener *listener)
{
	m_listener = listener;
}

int MidiControlServer::subscribe()
{
	snd_seq_drop_input(m_seq);
	snd_seq_drop_output(m_seq);
	return 0;
}

size_t MidiControlServer::getNumFds() const
{
	return snd_seq_poll_descriptors_count(m_seq, POLLIN);
}

int MidiControlServer::fillFds(struct pollfd *fds, size_t n) const
{
	if (n < getNumFds())
	{
		LOG_ERROR("Not enough space in fds array!\n");
		return -EINVAL;
	}

	return snd_seq_poll_descriptors(m_seq, fds, n, POLLIN);
}

int MidiControlServer::handleFdEvents(struct pollfd *fds, size_t nfds, size_t nevents)
{
	if (!nevents)
		return 0;

	unsigned short *revents = (unsigned short*)alloca(sizeof(unsigned short) * nfds);
	int err = snd_seq_poll_descriptors_revents(m_seq, fds, nfds, revents);
	if (err < 0)
		return err;

	size_t i=0, j=0;
	while (i < nfds && j < nevents)
	{
		if (revents[i] & POLLIN)
		{
			snd_seq_event_t *ev;
			err = snd_seq_event_input(m_seq, &ev);
			if (err >= 1 && m_listener)
			{
				handleEvent(ev);
			}
			++j;
		}
		++i;
	}

	return 0;
}

void MidiControlServer::handleEvent(const snd_seq_event_t *ev)
{
	int16_t value;
	control_id_t id = identify(value, ev);
	if (id < 0)
		return;

	auto range = m_controls.equal_range(id);
	for (auto itr = range.first; itr != range.second; ++itr)
	{
		itr->second.updateValue(value);
		m_listener->onControlChange(&itr->second);
	}
}

int MidiControlServer::sendEvent(MidiControlType type, int8_t channel, int8_t id, int16_t value)
{
	snd_seq_event_t ev;
	snd_seq_ev_clear(&ev);
	snd_seq_ev_set_source(&ev, m_port);
	snd_seq_ev_set_subs(&ev);
	snd_seq_ev_set_direct(&ev);

	switch (type)
	{
	case MCT_NOTE_ON:
		snd_seq_ev_set_noteon(&ev, channel, id, value);
		break;
	case MCT_NOTE_OFF:
		snd_seq_ev_set_noteoff(&ev, channel, id, value);
		break;
	case MCT_POLY_AFTERTOUCH:
		snd_seq_ev_set_keypress(&ev, channel, id, value);
		break;
	case MCT_CONTROL_CHANGE:
		snd_seq_ev_set_controller(&ev, channel, id, value);
		break;
	case MCT_PROGRAM_CHANGE:
		snd_seq_ev_set_pgmchange(&ev, channel, id);
		break;
	case MCT_CHANNEL_PRESSURE:
		snd_seq_ev_set_chanpress(&ev, channel, value);
		break;
	case MCT_PITCH_BEND:
		snd_seq_ev_set_pitchbend(&ev, channel, value);
		break;
	case MCT_START:
		snd_seq_ev_set_queue_start(&ev, 0);
		break;
	case MCT_CONTINUE:
		snd_seq_ev_set_queue_continue(&ev, 0);
		break;
	case MCT_STOP:
		snd_seq_ev_set_queue_stop(&ev, 0);
		break;
	case MCT_RESET:
		snd_seq_ev_set_fixed(&ev);
		ev.type = SND_SEQ_EVENT_RESET;
		break;
	default:
		return -EINVAL;
	}
	return snd_seq_event_output_direct(m_seq, &ev);
}

IControl *MidiControlServer::registerControl(const char *name, MidiControlType type, int8_t channel, int8_t id)
{
	control_id_t control_id = buildId(type, channel, id);
	auto r = m_controls.emplace(std::piecewise_construct, std::forward_as_tuple(control_id), std::forward_as_tuple(*this, name, type, channel, id));
	return &r->second;
}
