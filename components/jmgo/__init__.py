import esphome.codegen as cg
import esphome.config_validation as cv

# ESPHome external component for JMGO projector LAN + BLE control.
#
# Injects jmgo_lan_ble.h into the build, sets the projector IP during setup(),
# and enables CONFIG_BT_ENABLED + CONFIG_BT_NIMBLE_ENABLED automatically so
# users don't need sdkconfig_options in their YAML.

CODEOWNERS = []

CONFIG_SCHEMA = cv.Schema({
    cv.Required("projector_ip"): cv.string,
})

async def to_code(config):
    cg.add_global(cg.RawExpression('#include "jmgo_lan_ble.h"'))
    ip = config["projector_ip"]
    cg.add(cg.RawExpression(f'jmgo_set_projector_ip("{ip}")'))

    # Enable BLE sdkconfig options when using ESP-IDF.
    # Wrapped in try/except for compatibility across ESPHome versions —
    # CORE.using_esp_idf was removed in newer releases.
    try:
        from esphome.components.esp32 import add_idf_sdkconfig_option
        add_idf_sdkconfig_option("CONFIG_BT_ENABLED", True)
        add_idf_sdkconfig_option("CONFIG_BT_NIMBLE_ENABLED", True)
    except Exception:
        pass
