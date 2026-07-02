import esphome.codegen as cg
import esphome.config_validation as cv

# ESPHome external component for JMGO projector LAN + BLE control.
#
# Injects jmgo_lan_ble.h into the build and calls jmgo_set_projector_ip()
# during setup().
#
# NimBLE (used for BLE wake) requires CONFIG_BT_ENABLED and
# CONFIG_BT_NIMBLE_ENABLED in your esp32: sdkconfig_options.
# This component intentionally does NOT set those options — doing so
# via the Python API conflicts with other BLE stacks (e.g. bluetooth_proxy
# uses Bluedroid, not NimBLE) and the correct values depend on your device.

CODEOWNERS = []

CONFIG_SCHEMA = cv.Schema({
    cv.Required("projector_ip"): cv.string,
})

async def to_code(config):
    # RawStatement (not RawExpression) avoids add_global wrapping it in
    # ExpressionStatement which appends ";". #include must not have one.
    # Full path because ESPHome's include root is src/ and external components
    # land at src/esphome/components/<name>/.
    cg.add_global(cg.RawStatement('#include "esphome/components/jmgo/jmgo_lan_ble.h"'))
    ip = config["projector_ip"]
    cg.add(cg.RawExpression(f'jmgo_set_projector_ip("{ip}")'))
