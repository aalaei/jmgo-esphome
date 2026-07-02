# JMGO Projector — ESPHome Package

Control a JMGO projector from Home Assistant using an ESP32-C3 over Wi-Fi.
Sends key events via LAN (TCP port 9005) and wakes the projector via BLE advertising.

## Features

- Power switch with real state detection (TCP probe every 5 s)
- Full navigation: Up / Down / Left / Right / OK / Back / Home / Menu
- Volume Up / Down
- Play, Quick Settings, Calibrate, Sleep
- BLE wake burst for cold-power-on
- All key IDs are overridable via YAML substitutions — no C++ edits needed

## Usage

Minimum YAML to control your projector (see `example.yaml` for the full version):

```yaml
substitutions:
  projector_ip: "192.168.1.100"   # your projector's static IP

packages:
  jmgo: github://aalaei/jmgo-esphome/jmgo_package.yaml@main

esphome:
  name: jmgo-projector
  friendly_name: "JMGO Projector"

esp32:
  board: esp32-c3-devkitm-1
  framework:
    type: esp-idf

wifi:
  ssid: !secret wifi_ssid
  password: !secret wifi_password

api:
  encryption:
    key: !secret api_key

ota:
  - platform: esphome
    password: !secret ota_password

logger:
```

That's it. ESPHome pulls the package and the C++ driver from this repo automatically.

## Customising key codes

Every key ID is a substitution with a default value. Override any of them in your YAML:

```yaml
substitutions:
  projector_ip:       "192.168.1.100"
  key_sleep:          "2011"   # standby key — change if your model differs
  key_quick_settings: "145"    # quick settings panel
  key_calibrate:      "2012"   # lens calibration
```

## BLE wake

The BLE wake payloads in `jmgo_lan_ble.h` encode the projector's BLE MAC address.
The default is for the JMGO N1S 4K (MAC `F0:ED:51:3D:14:0D`).

If BLE wake doesn't work on your projector:
1. Install **nRF Connect** on your phone and stand near the powered-off projector.
2. Press the projector's physical power button — note the advertised BLE MAC.
3. Reverse the bytes (e.g. `E0:D8:C4:98:C5:69` → `0x69,0xC5,0x98,0xC4,0xD8,0xE0`).
4. Fork this repo, update bytes 3-8 in all 5 rows of `JMGO_WAKE_PAYLOADS` in
   `components/jmgo/jmgo_lan_ble.h`, and point your package URL at your fork.

## Key codes reference

| Key ID | Function | Source |
|--------|----------|--------|
| 3      | Home | confirmed |
| 4      | Back | confirmed |
| 19     | D-pad Up | confirmed |
| 20     | D-pad Down | confirmed |
| 21     | D-pad Left | confirmed |
| 22     | D-pad Right | confirmed |
| 23     | OK / Center | confirmed |
| 24     | Volume Up | confirmed |
| 25     | Volume Down | confirmed |
| 82     | Menu | confirmed |
| 126    | Media Play | confirmed |
| 145    | Quick Settings | confirmed (PCAP) |
| 2011   | Sleep / Standby | confirmed |
| 2012   | Calibrate | confirmed (PCAP) |

Key codes were extracted from PCAPdroid captures of the official JMGO Android app
and confirmed against the original [JMGO-MQTT-Remote](https://github.com/akantifirst/JMGO-MQTT-Remote)
firmware by [@akantifirst](https://github.com/akantifirst), whose reverse-engineering
work on the LAN protocol and BLE wake mechanism made this package possible.

## Requirements

- ESP32-C3 (BLE + Wi-Fi)
- ESPHome 2024.6 or newer
- Projector on the same LAN with a static IP
