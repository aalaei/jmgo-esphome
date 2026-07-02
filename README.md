# JMGO Projector — ESPHome Package

Control a JMGO projector from Home Assistant using an ESP32 over Wi-Fi.
Sends key events via LAN (TCP port 9005) and wakes the projector via BLE advertising.
Supports ESP32-C3, C6, S3, C5, and classic ESP32.

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
  device_name:    "jmgo-n1s-4k"     # ESPHome node ID (lowercase, hyphens) — for OTA and mDNS
  projector_name: "N1S 4K"          # HA device display name — entities appear under this device
  projector_ip:   "192.168.1.100"   # projector static IP (must override)

packages:
  jmgo: github://aalaei/jmgo-esphome/jmgo_package.yaml@main

esp32:
  board: esp32-c3-devkitm-1   # change to match your chip — see Requirements
  framework:
    type: esp-idf
  # NimBLE sdkconfig options (CONFIG_BT_ENABLED, CONFIG_BT_NIMBLE_ENABLED)
  # are injected automatically by the package — no extra lines needed here.

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

ESPHome pulls the package and the C++ driver from this repo automatically —
no `.h` file to download manually.

> **Note:** Do **not** combine this package with a `bluetooth_proxy` on the same device.
> Bluetooth proxy uses the Bluedroid BLE stack, which conflicts with NimBLE (used for BLE wake).

## How entities appear in Home Assistant

The package intentionally omits the `esphome:` block so your own config's
`devices:` and `areas:` definitions survive the package merge without conflict.
You must include `esphome:` in your device YAML.

### Simple setup — one HA device

```yaml
esphome:
  name: ${device_name}
  friendly_name: ${projector_name}   # projector entities appear under this device
```

### Sub-device setup — projector as a child device

Define the projector as a sub-device and set `projector_id` to the same `id`:

```yaml
substitutions:
  device_name:    "esplux"
  projector_name: "JMGO N1S 4K"
  projector_id:   "jmgo_n1s_4k"     # must match devices: id below
  projector_ip:   "192.168.11.207"

esphome:
  name: ${device_name}
  friendly_name: "ESP LUX"           # ESP32 board device name
  devices:
    - id: ${projector_id}
      name: ${projector_name}
```

All 18 projector entities (Power, Up, Down …) will appear under the
**JMGO N1S 4K** sub-device in HA. The ESP32 board itself remains **ESP LUX**.

## Substitutions reference

| Substitution | Default | Description |
|---|---|---|
| `device_name` | `jmgo-projector` | ESPHome node hostname — used for OTA and mDNS only |
| `projector_name` | `Projector` | HA device display name — all entities are grouped under it |
| `projector_id` | _(empty)_ | HA device ID to bind all entities to (copy from the device page URL in HA) |
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

- Any ESP32 variant with BLE + Wi-Fi running ESP-IDF framework:

  | Chip | Board name for `esp32: board:` |
  |------|-------------------------------|
  | ESP32-C3 | `esp32-c3-devkitm-1` |
  | ESP32-S3 | `esp32-s3-devkitc-1` |
  | ESP32-C6 | `esp32-c6-devkitc-1` |
  | ESP32-C5 | `esp32-c5-devkitc-1` |
  | Classic ESP32 | `esp32dev` |

  The C++ driver uses only NimBLE + lwIP POSIX sockets — no chip-specific code.

- ESPHome 2024.6 or newer (tested on 2026.6.1)
- Projector on the same LAN with a static IP
