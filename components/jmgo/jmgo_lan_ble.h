#pragma once
// JMGO projector LAN + BLE wake helpers for ESPHome.
// Framework: ESP-IDF (NimBLE via IDF native stack).
// BLE is initialized lazily on the first wake burst, after WiFi setup
// is complete - avoids RF coexistence init ordering issues on ESP32-C3.
// TCP uses POSIX sockets via lwIP (no WiFi.h needed).
//
// Key IDs and macros live in the YAML (substitutions + scripts).
// This file only provides jmgo_key(), jmgo_lan_send(), and BLE wake.

#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/ble_gap.h"
#include "lwip/sockets.h"
#include "lwip/inet.h"
#include <fcntl.h>
#include <errno.h>
#include <string.h>

// BLE wake advertising payloads.
// Bytes 3-8 are the projector BLE MAC in reverse byte order.
// If your projector does not wake up, sniff its BLE MAC with nRF Connect,
// reverse the bytes, and replace all 6 bytes in every row.
static const uint8_t JMGO_WAKE_PAYLOADS[][14] = {
  {0x46,0x00,0x01,0x0D,0x14,0x3D,0x51,0xED,0xF0,0xff,0xff,0xff,0xff,0xff},
  {0x46,0x00,0x02,0x0D,0x14,0x3D,0x51,0xED,0xF0,0xff,0xff,0xff,0xff,0xff},
  {0x46,0x00,0x03,0x0D,0x14,0x3D,0x51,0xED,0xF0,0xff,0xff,0xff,0xff,0xff},
  {0x46,0x00,0x04,0x0D,0x14,0x3D,0x51,0xED,0xF0,0xff,0xff,0xff,0xff,0xff},
  {0x46,0x00,0x05,0x0D,0x14,0x3D,0x51,0xED,0xF0,0xff,0xff,0xff,0xff,0xff},
};

// TCP/LAN via POSIX sockets (lwIP).

static uint32_t jmgo_projector_ip_u32 = 0;
static const uint16_t JMGO_LAN_PORT = 9005;

static void jmgo_set_projector_ip(const char* ip_str) {
  jmgo_projector_ip_u32 = inet_addr(ip_str);
}

static bool jmgo_lan_send(const uint8_t* data, size_t len) {
  if (jmgo_projector_ip_u32 == 0) return false;
  // :: prefix avoids collision with esphome::socket namespace
  int fd = ::socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) return false;

  int flags = ::fcntl(fd, F_GETFL, 0);
  ::fcntl(fd, F_SETFL, flags | O_NONBLOCK);

  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(JMGO_LAN_PORT);
  addr.sin_addr.s_addr = jmgo_projector_ip_u32;

  int r = ::connect(fd, (struct sockaddr*)&addr, sizeof(addr));
  if (r < 0 && errno != EINPROGRESS) { ::close(fd); return false; }

  fd_set wfds;
  FD_ZERO(&wfds);
  FD_SET(fd, &wfds);
  struct timeval tv = {3, 0};
  r = ::select(fd + 1, NULL, &wfds, NULL, &tv);
  if (r <= 0) { ::close(fd); return false; }

  int err = 0;
  socklen_t errlen = sizeof(err);
  ::getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &errlen);
  if (err != 0) { ::close(fd); return false; }

  ::fcntl(fd, F_SETFL, flags);
  struct timeval stv = {3, 0};
  ::setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &stv, sizeof(stv));

  int n = ::send(fd, data, len, 0);
  ::close(fd);
  return n == (int)len;
}

// Generic key press+release. Builds protobuf packet from key_id.
// Packet layout (key_id < 128, 9 bytes):
//   [total_len, 0x12, inner_len, 0x0a, iii_len, 0x08, key_id, 0x10, press]
// For key_id >= 128 a 2-byte varint is used (10 bytes total).
static void jmgo_key(uint16_t key_id, uint32_t hold_ms = 120, uint32_t after_ms = 120) {
  if (key_id < 128) {
    uint8_t pkt[] = {0x08, 0x12, 0x06, 0x0a, 0x04, 0x08, (uint8_t)key_id, 0x10, 0x01};
    jmgo_lan_send(pkt, sizeof(pkt));
    esphome::delay(hold_ms);
    pkt[8] = 0x00;
    jmgo_lan_send(pkt, sizeof(pkt));
  } else {
    uint8_t pkt[] = {0x09, 0x12, 0x07, 0x0a, 0x05, 0x08,
                     (uint8_t)((key_id & 0x7f) | 0x80),
                     (uint8_t)(key_id >> 7),
                     0x10, 0x01};
    jmgo_lan_send(pkt, sizeof(pkt));
    esphome::delay(hold_ms);
    pkt[9] = 0x00;
    jmgo_lan_send(pkt, sizeof(pkt));
  }
  esphome::delay(after_ms);
}

// BLE wake (IDF native NimBLE - lazy init).
// NimBLE is initialized lazily (first call to jmgo_ble_wake_burst) so it
// never runs during ESPHome setup(), avoiding RF coexistence init issues.

static volatile bool jmgo_nimble_synced = false;
static bool jmgo_ble_started = false;

static void jmgo_ble_on_sync(void) { jmgo_nimble_synced = true; }

static void jmgo_ble_host_task(void* param) {
  nimble_port_run();
  nimble_port_freertos_deinit();
}

static void jmgo_ble_init() {
  if (jmgo_ble_started) return;
  jmgo_ble_started = true;

  esp_err_t ret = nimble_port_init();
  if (ret != ESP_OK) {
    ESP_LOGE("jmgo", "nimble_port_init failed: %d", ret);
    return;
  }
  ble_hs_cfg.sync_cb = jmgo_ble_on_sync;
  nimble_port_freertos_init(jmgo_ble_host_task);

  uint32_t t0 = esphome::millis();
  while (!jmgo_nimble_synced && esphome::millis() - t0 < 5000) esphome::delay(10);
  if (!jmgo_nimble_synced)
    ESP_LOGE("jmgo", "NimBLE host did not sync within 5 s");
}

static void jmgo_ble_advertise_one(const uint8_t* mfr, size_t mfr_len) {
  uint8_t adv_raw[31];
  adv_raw[0] = (uint8_t)(1 + mfr_len);
  adv_raw[1] = 0xFF;
  memcpy(&adv_raw[2], mfr, mfr_len);
  ble_gap_adv_set_data(adv_raw, (int)(2 + mfr_len));
  struct ble_gap_adv_params params;
  memset(&params, 0, sizeof(params));
  params.conn_mode = BLE_GAP_CONN_MODE_NON;
  params.disc_mode = BLE_GAP_DISC_MODE_NON;
  params.itvl_min = 0x20;
  params.itvl_max = 0x40;
  ble_gap_adv_start(BLE_OWN_ADDR_PUBLIC, NULL, BLE_HS_FOREVER, &params, NULL, NULL);
  esphome::delay(250);
  ble_gap_adv_stop();
  esphome::delay(30);
}

// Blocks ~4 s (plus up to 5 s on the very first call for NimBLE host sync).
static void jmgo_ble_wake_burst() {
  jmgo_ble_init();
  if (!jmgo_nimble_synced) {
    ESP_LOGE("jmgo", "BLE not ready, skipping wake burst");
    return;
  }
  uint32_t start = esphome::millis();
  while (esphome::millis() - start < 4000) {
    for (int i = 0; i < 5; i++) {
      jmgo_ble_advertise_one(JMGO_WAKE_PAYLOADS[i], sizeof(JMGO_WAKE_PAYLOADS[i]));
      if (esphome::millis() - start >= 4000) break;
    }
  }
  ble_gap_adv_stop();
}
