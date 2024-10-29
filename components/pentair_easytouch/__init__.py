import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor, uart, switch
from esphome.const import (
    CONF_ID,
    CONF_PIN,
    CONF_NAME,
    CONF_DISABLED_BY_DEFAULT,
    CONF_ICON,
    CONF_INVERTED,
    CONF_TEMPERATURE,
    DEVICE_CLASS_TEMPERATURE,
    ICON_THERMOMETER,
)
from esphome.cpp_helpers import gpio_pin_expression
from esphome import pins

CODEOWNERS = ["@michaelusner"]
MULTI_CONF = True
DEPENDENCIES = ["uart"]
AUTO_LOAD = ["sensor", "switch"]

# Configuration constants
CONF_AIR_TEMPERATURE_SENSOR = "air_temperature_sensor"
CONF_WATER_TEMPERATURE_SENSOR = "water_temperature_sensor"
CONF_FLOW_CONTROL_PIN = "flow_control_pin"
CONF_FEATURE = "feature"

# Feature configurations with switch suffix
SWITCH_CONFIGS = {
    "pool_switch": 6,      # POOL
    "spa_switch": 1,       # SPA
    "cleaner_switch": 2,   # CLEANER
    "air_blower_switch": 3, # AIR_BLOWER
    "spa_light_switch": 4,  # SPA_LIGHT
    "pool_light_switch": 5, # POOL_LIGHT
    "water_feature_switch": 7, # WATER_FEATURE
    "spillway_switch": 8,  # SPILLWAY
}

pentair_easytouch_component_ns = cg.esphome_ns.namespace("pentair_easytouch")
PentairEasyTouchComponent = pentair_easytouch_component_ns.class_(
    "PentairEasyTouchComponent", cg.Component, uart.UARTDevice
)

UARTSwitch = pentair_easytouch_component_ns.class_(
    "UARTSwitch", switch.Switch, cg.Component
)

def create_switch_schema():
    return switch.switch_schema(
        UARTSwitch,
        icon="mdi:pool",
    ).extend({
        cv.Required(CONF_FEATURE): cv.enum(
            {k.replace("_switch", ""): v for k, v in SWITCH_CONFIGS.items()},
            lower=True
        ),
    })

SWITCH_SCHEMAS = {
    switch_name: create_switch_schema()
    for switch_name in SWITCH_CONFIGS.keys()
}

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(PentairEasyTouchComponent),
            cv.Required(CONF_FLOW_CONTROL_PIN): pins.gpio_output_pin_schema,
            cv.Optional(CONF_AIR_TEMPERATURE_SENSOR): sensor.sensor_schema(
                unit_of_measurement="°F",
                icon=ICON_THERMOMETER,
                accuracy_decimals=0,
                device_class=DEVICE_CLASS_TEMPERATURE,
            ),
            cv.Optional(CONF_WATER_TEMPERATURE_SENSOR): sensor.sensor_schema(
                unit_of_measurement="°F",
                icon=ICON_THERMOMETER,
                accuracy_decimals=0,
                device_class=DEVICE_CLASS_TEMPERATURE,
            ),
            **{
                cv.Optional(switch_name): schema
                for switch_name, schema in SWITCH_SCHEMAS.items()
            }
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(uart.UART_DEVICE_SCHEMA)
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)

    if CONF_AIR_TEMPERATURE_SENSOR in config:
        sens = await sensor.new_sensor(config[CONF_AIR_TEMPERATURE_SENSOR])
        cg.add(var.set_air_temperature_sensor(sens))
    
    if CONF_WATER_TEMPERATURE_SENSOR in config:
        sens = await sensor.new_sensor(config[CONF_WATER_TEMPERATURE_SENSOR])
        cg.add(var.set_water_temperature_sensor(sens))
    
    if CONF_FLOW_CONTROL_PIN in config:
        pin = await gpio_pin_expression(config[CONF_FLOW_CONTROL_PIN])
        cg.add(var.set_flow_control_pin(pin))

    # Register all configured switches
    for switch_name in SWITCH_CONFIGS.keys():
        if switch_name in config:
            conf = config[switch_name]
            sw = cg.new_Pvariable(conf[CONF_ID])
            await switch.register_switch(sw, conf)
            cg.add(sw.set_parent(var))
            cg.add(sw.set_feature(SWITCH_CONFIGS[switch_name]))  # Using int_to_feature in C++