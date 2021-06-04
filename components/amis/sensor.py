import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart, sensor
from esphome.const import (
    CONF_ID,
    DEVICE_CLASS_POWER,
    DEVICE_CLASS_ENERGY,
    ICON_EMPTY,
    UNIT_WATT,
    UNIT_WATT_HOURS,
#    UNIT_VOLT_AMPS_REACTIVE_HOURS,
    UNIT_VOLT_AMPS_REACTIVE,
)

DEPENDENCIES = ["uart"]

amis_ns = cg.esphome_ns.namespace("amis")
AMISComponent = amis_ns.class_("AMISComponent", cg.Component, uart.UARTDevice)

CONF_POWER_GRID_KEY = 'power_grid_key'
CONF_ENERGY_A_POSITIVE = 'energy_a_positive'
CONF_ENERGY_A_NEGATIVE = 'energy_a_negative'
CONF_REACTIVE_ENERGY_A_POSITIVE = 'reactive_energy_a_positive'
CONF_REACTIVE_ENERGY_A_NEGATIVE = 'reactive_energy_a_negative'
CONF_INSTANTANEOUS_POWER_A_POSITIVE = 'instantaneous_power_a_positive'
CONF_INSTANTANEOUS_POWER_A_NEGATIVE = 'instantaneous_power_a_negative'
CONF_REACTIVE_INSTANTANEOUS_POWER_A_POSITIVE = 'reactive_instantaneous_power_a_positive'
CONF_REACTIVE_INSTANTANEOUS_POWER_A_NEGATIVE = 'reactive_instantaneous_power_a_negative'

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.Required(CONF_POWER_GRID_KEY): cv.string, 
            cv.GenerateID(): cv.declare_id(AMISComponent),
            cv.Optional(CONF_ENERGY_A_POSITIVE): sensor.sensor_schema(
                UNIT_WATT_HOURS, ICON_EMPTY, 1, DEVICE_CLASS_ENERGY
            ),
            cv.Optional(CONF_ENERGY_A_NEGATIVE): sensor.sensor_schema(
                UNIT_WATT_HOURS, ICON_EMPTY, 1, DEVICE_CLASS_ENERGY
            ),
#            cv.Optional(CONF_REACTIVE_ENERGY_A_POSITIVE): sensor.sensor_schema(
#                UNIT_VOLT_AMPS_REACTIVE_HOURS, ICON_EMPTY, 1, DEVICE_CLASS_ENERGY
#            ),
#            cv.Optional(CONF_REACTIVE_ENERGY_A_NEGATIVE): sensor.sensor_schema(
#                UNIT_VOLT_AMPS_REACTIVE_HOURS, ICON_EMPTY, 1, DEVICE_CLASS_ENERGY
#            ),
            cv.Optional(CONF_INSTANTANEOUS_POWER_A_POSITIVE): sensor.sensor_schema(
                UNIT_WATT, ICON_EMPTY, 1, DEVICE_CLASS_POWER
            ),
            cv.Optional(CONF_INSTANTANEOUS_POWER_A_NEGATIVE): sensor.sensor_schema(
                UNIT_WATT, ICON_EMPTY, 1, DEVICE_CLASS_POWER
            ),
            cv.Optional(CONF_REACTIVE_INSTANTANEOUS_POWER_A_POSITIVE): sensor.sensor_schema(
                UNIT_VOLT_AMPS_REACTIVE, ICON_EMPTY, 1, DEVICE_CLASS_POWER
            ),
            cv.Optional(CONF_REACTIVE_INSTANTANEOUS_POWER_A_NEGATIVE): sensor.sensor_schema(
                UNIT_VOLT_AMPS_REACTIVE, ICON_EMPTY, 1, DEVICE_CLASS_POWER
            ),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(uart.UART_DEVICE_SCHEMA)
)


def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    yield cg.register_component(var, config)
    yield uart.register_uart_device(var, config)
    
    cg.add(var.set_power_grid_key(config[CONF_POWER_GRID_KEY]))
    
    for key in [
        CONF_ENERGY_A_POSITIVE,
        CONF_ENERGY_A_NEGATIVE,
        CONF_REACTIVE_ENERGY_A_POSITIVE,
        CONF_REACTIVE_ENERGY_A_NEGATIVE,
        CONF_INSTANTANEOUS_POWER_A_POSITIVE,
        CONF_INSTANTANEOUS_POWER_A_NEGATIVE,
        CONF_REACTIVE_INSTANTANEOUS_POWER_A_POSITIVE,
        CONF_REACTIVE_INSTANTANEOUS_POWER_A_NEGATIVE,
    ]:
        if not key in config:
            continue
        conf = config[key]
        sens = yield sensor.new_sensor(conf)
        cg.add(getattr(var, f"set_{key}_sensor")(sens))
