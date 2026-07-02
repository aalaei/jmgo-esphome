import esphome.codegen as cg
import esphome.config_validation as cv

# ESPHome external component for JMGO projector LAN + BLE control.
#
# Injects jmgo_lan_ble.h into the build and calls jmgo_set_projector_ip()
# during setup() so that all lambdas in the package YAML can call
# jmgo_key(), jmgo_lan_send(), and jmgo_ble_wake_burst() directly.

CODEOWNERS = []

CONFIG_SCHEMA = cv.Schema({
    cv.Required("projector_ip"): cv.string,
})

async def to_code(config):
    cg.add_global(cg.RawExpression('#include "jmgo_lan_ble.h"'))
    ip = config["projector_ip"]
    cg.add(cg.RawExpression(f'jmgo_set_projector_ip("{ip}")'))
