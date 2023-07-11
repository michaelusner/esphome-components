import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor, uart, switch
from esphome.const import (
    CONF_ID,
    CONF_PIN,
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
CONF_AIR_TEMPERATURE_SENSOR = "air_temperature_sensor"
CONF_WATER_TEMPERATURE_SENSOR = "water_temperature_sensor"
CONF_FLOW_CONTROL_PIN = "flow_control_pin"
CONF_PUMP1_SWITCH = "pump1_switch"

pentair_easytouch_component_ns = cg.esphome_ns.namespace("pentair_easytouch")
PentairEasyTouchComponent = pentair_easytouch_component_ns.class_(
    "PentairEasyTouchComponent", cg.Component, uart.UARTDevice
)

Pump1Switch = pentair_easytouch_component_ns.class_(
    "UARTSwitch", switch.Switch, cg.Component
)
CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(PentairEasyTouchComponent),
            cv.Required(CONF_FLOW_CONTROL_PIN): pins.gpio_output_pin_schema,
            cv.Optional(CONF_AIR_TEMPERATURE_SENSOR): sensor.sensor_schema(
                unit_of_measurement="F",
                icon=ICON_THERMOMETER,
                accuracy_decimals=0,
                device_class=DEVICE_CLASS_TEMPERATURE,
            ),
            cv.Optional(CONF_WATER_TEMPERATURE_SENSOR): sensor.sensor_schema(
                unit_of_measurement="F",
                icon=ICON_THERMOMETER,
                accuracy_decimals=0,
                device_class=DEVICE_CLASS_TEMPERATURE,
            ),
            cv.Optional(CONF_PUMP1_SWITCH): switch.SWITCH_SCHEMA.extend(
                {cv.GenerateID(): cv.declare_id(Pump1Switch)},
                {cv.Required("address"): cv.int_}
            ),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(uart.UART_DEVICE_SCHEMA)
)


async def to_code(config):
    print(f"config: {config}")
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)
    if CONF_AIR_TEMPERATURE_SENSOR in config:
        air_temp_sensor = await sensor.new_sensor(config[CONF_AIR_TEMPERATURE_SENSOR])
        cg.add(var.set_air_temperature_sensor(air_temp_sensor))
    if CONF_WATER_TEMPERATURE_SENSOR in config:
        water_temp_sensor = await sensor.new_sensor(
            config[CONF_WATER_TEMPERATURE_SENSOR]
        )
        cg.add(var.set_water_temperature_sensor(water_temp_sensor))
    if CONF_FLOW_CONTROL_PIN in config:
        pin = await gpio_pin_expression(config[CONF_FLOW_CONTROL_PIN])
        cg.add(var.set_flow_control_pin(pin))
    if CONF_PUMP1_SWITCH in config:
        conf = config[CONF_PUMP1_SWITCH]
        sw = cg.new_Pvariable(conf[CONF_ID])
        await switch.register_switch(sw, conf)
        cg.add(sw.set_parent(var))
        cg.add(sw.set_address(config["pump1_switch"]["address"]))
