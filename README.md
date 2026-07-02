# JMGO Projector — ESPHome Package

Control a JMGO projector from Home Assistant using an ESP32-C3 over Wi-Fi.
Sends key events via LAN (TCP port 9005) and wakes the projector via BLE advertising.

## Features

- Power switch with real state detection (TCP probe every 5 s)
- Full navigation: Up / Down / Left / Right / OK / Back / Home / Menu
- Volume Up / Down
- Play, Quick Settings, Calibrate, Sleep
- BLE wake burst for cold-power-on
- All key IDs and entity names are overridable via YAML substitutions — no C++ edits needed

## Usage

Minimum YAML to control your projector (see `example.yaml` for the full version):

```yaml
substitutions:
  device_name:    "jmgo-projector"      # ESPHome node ID (lowercase, hyphens only)
  friendly_name:  "JMGO Controller"     # HA device name for the ESP32 hardware
  projector_name: "N1S 4K"             # label bound to every entity — set per projector
  projector_ip:   "192.168.1.100"       # projector static IP (must override)

packages:
  jmgo: github://aalaei/jmgo-esphome/jmgo_package.yaml@main

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

That's it. ESPHome pulls the package and the C++ driver from this repo automatically —
no `.h` file to download manually, no `sdkconfig_options` to set.

## Entity naming

`projector_name` is prepended to every entity exposed to Home Assistant:

| `projector_name` value | Example entity names in HA |
|------------------------|---------------------------|
| `"N1S 4K"` | N1S 4K Power, N1S 4K Up, N1S 4K Online … |
| `"Living Room"` | Living Room Power, Living Room Up … |
| `"Bedroom"` | Bedroom Power, Bedroom Up … |

This means two projectors controlled by the same (or different) ESP32 nodes will
never have colliding entity names in Home Assistant.

## Substitutions reference

| Substitution | Default | Description |
|---|---|---|
| `device_name` | `jmgo-projector` | ESPHome node hostname — used for OTA and mDNS |
| `friendly_name` | `JMGO Projector` | HA device name for the ESP32 hardware |
| `projector_name` | `Projector` | Prefix for every entity name in HA |
| `projector_ip` | `192.168.1.100` | Projector LAN IP — **must override** |
| `key_up` | `19` | D-pad Up |
| `key_down` | `20` | D-pad Down |
| `key_left` | `21` | D-pad Left |
| `key_right` | `22` | D-pad Right |
| `key_ok` | `23` | OK / Center |
| `key_back` | `4` | Back |
| `key_home` | `3` | Home |
| `key_menu` | `82` | Menu |
| `key_vol_up` | `24` | Volume Up |
| `key_vol_down` | `25` | Volume Down |
| `key_play` | `126` | Media Play |
| `key_quick_settings` | `145` | Quick Settings panel |
| `key_calibrate` | `2012` | Lens / image calibration |
| `key_sleep` | `2011` | Sleep / Standby |

## Customising key codes

Override any key ID in your substitutions block:

```yaml
substitutions:
  projector_ip:       "192.168.11.207"
  key_sleep:          "2011"   # change if your model uses a different code
  key_quick_settings: "145"
  key_calibrate:      "2012"
```

## BLE wake

The BLE wake payloads encode the projector's BLE MAC address.
The default is for the JMGO N1S 4K (MAC `F0:ED:51:3D:14:0D`).

If BLE wake doesn't work on your projector:
1. Install **nRF Connect** on your phone near the powered-off projector.
2. Press the projector's physical power button — note the advertised BLE MAC.
3. Reverse the bytes (e.g. `E0:D8:C4:98:C5:69` → `0x69,0xC5,0x98,0xC4,0xD8,0xE0`).
4. Fork this repo, update bytes 3-8 in all 5 rows of `JMGO_WAKE_PAYLOADS` in
   `components/jmgo/jmgo_lan_ble.h`, and point your `packages:` URL at your fork.

## Key codes reference

All codes confirmed from PCAPdroid captures of the official JMGO Android app.

| Key ID | Function |
|--------|----------|
| 3 | Home |
| 4 | Back |
| 19 | D-pad Up |
| 20 | D-pad Down |
| 21 | D-pad Left |
| 22 | D-pad Right |
| 23 | OK / Center |
| 24 | Volume Up |
| 25 | Volume Down |
| 82 | Menu |
| 126 | Media Play |
| 145 | Quick Settings |
| 2011 | Sleep / Standby |
| 2012 | Calibrate |

## Credits

Key codes were extracted from PCAPdroid captures of the official JMGO Android app
and cross-referenced against the original [JMGO-MQTT-Remote](https://github.com/akantifirst/JMGO-MQTT-Remote)
firmware by [@akantifirst](https://github.com/akantifirst), whose reverse-engineering
of the LAN protocol and BLE wake mechanism made this package possible.

## Requirements

- ESP32-C3 (BLE + Wi-Fi, other ESP32 variants with BLE should also work)
- ESPHome 2024.6 or newer
- Projector on the same LAN with a static IP
