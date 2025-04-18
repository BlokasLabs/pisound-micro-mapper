# Pisound Micro Mapper

Pisound Micro Mapper is a program running in the background that takes care of initializing [Pisound Micro](https://blokas.io/pisound-micro/) controls according to the provided configuration file. It can translate hardware controls to MIDI, OSC, or other common audio software communication means as well as map them to ALSA mixer controls.

## Features

- Configure and initialize Pisound Micro hardware controls (encoders, analog inputs, GPIO inputs/outputs).
- Map hardware controls to ALSA mixer controls (volume, gain, mute, etc.) for direct audio parameter manipulation.
- Create virtual MIDI ports with configurable MIDI messages.
- Set up OSC (Open Sound Control) endpoints for communication with other software.
- Bidirectional mapping between different control systems.

## Installation

If you're using Patchbox OS or have set up the Blokas APT server, you can install using:

```bash
sudo apt install -y pisound-micro-mapper
```

## Configuration

The configuration is defined in a JSON file, located by default at `/etc/pisound-micro-mapper.json`. For detailed configuration documentation, please refer to the [Pisound Micro docs](https://blokas.io/pisound-micro/docs/pisound-micro-mapper/).

After modifying the configuration, restart the service:

```bash
sudo systemctl restart pisound-micro-mapper.service
```

### Basic Example

Here's a minimal working configuration that maps a rotary encoder to the Digital Playback Volume control:

```json
{
    "$schema": "https://blokas.io/json/pisound-micro-schema.json",
    "version": 1,
    "controls": {
        "pisound-micro": {
            "volume_encoder": {
                "type": "encoder",
                "pins": [ "B03", "pull_up", "B04", "pull_up" ]
            }
        },
        "alsa": {
            "hw:pisoundmicro": [
                "Digital Playback Volume"
            ]
        }
    },
    "mappings": [
        [ "Digital Playback Volume", "<->", "volume_encoder" ]
    ]
}
```

## Control Types

### Pisound Micro Controls

- **Encoder**: Rotary encoder controls
- **Analog Input**: Analog inputs like potentiometers
- **GPIO Input**: Binary input controls
- **GPIO Output**: Control LEDs or other outputs
- **Activity**: Status indicators for MIDI activity

### ALSA Controls

Interface with the ALSA mixer controls, such as volume controls. To discover available controls:

```bash
amixer -D hw:pisoundmicro controls
```

### MIDI Controls

Create virtual MIDI ports and map controls to MIDI messages:
- Note On/Off
- Control Change
- Program Change
- Pitch Bend
- Channel Pressure
- Transport controls (Start, Stop, Continue)

### OSC Controls

Define OSC endpoints for communication with other software using the Open Sound Control protocol.

## Mapping Controls

Controls are mapped using the `mappings` section in the configuration file. The syntax is:

```json
[ "ControlA", "Direction", "ControlB", { "Options" } ]
```

Where direction can be:
- `->`: Map from A to B
- `<-`: Map from B to A
- `<->`: Bidirectional mapping

## Command Line Options

```
pisound-micro-mapper [--config <config.json>]

--config <config.json>      Load the config from the specified file. Default: /etc/pisound-micro-mapper.json
--help                      Print help
--version                   Print version
```

## Learn More

For detailed information about configuration options and schema reference, visit:
[Pisound Micro Mapper Documentation](https://blokas.io/pisound-micro/docs/pisound-micro-mapper/)

## License

Pisound Micro Mapper © Blokas https://blokas.io/
