import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import (
    CONF_ID,
    CONF_TEMPERATURE,
    CONF_HUMIDITY,
    CONF_UPDATE_INTERVAL,
    UNIT_CELSIUS,
    UNIT_PERCENT,
    DEVICE_CLASS_TEMPERATURE,
    DEVICE_CLASS_HUMIDITY,
    ICON_THERMOMETER,
    ICON_WATER_PERCENT,
)

CONF_SDA_PIN = "sda_pin"
CONF_SCL_PIN = "scl_pin"

cht8305_ns = cg.esphome_ns.namespace("cht8305_sniffer")
CHT8305Sniffer = cht8305_ns.class_("CHT8305SnifferSensor", cg.PollingComponent)

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(CHT8305Sniffer),
    cv.Optional(CONF_SDA_PIN, default=8): cv.int_,
    cv.Optional(CONF_SCL_PIN, default=9): cv.int_,
    cv.Optional(CONF_TEMPERATURE, default={}): sensor.sensor_schema(
        unit_of_measurement=UNIT_CELSIUS,
        icon=ICON_THERMOMETER,
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_TEMPERATURE,
        state_class="measurement",
    ),
    cv.Optional(CONF_HUMIDITY, default={}): sensor.sensor_schema(
        unit_of_measurement=UNIT_PERCENT,
        icon=ICON_WATER_PERCENT,
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_HUMIDITY,
        state_class="measurement",
    )
}).extend(cv.polling_component_schema("10s"))

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    cg.add(var.set_sda_pin(config[CONF_SDA_PIN]))
    cg.add(var.set_scl_pin(config[CONF_SCL_PIN]))

    if CONF_TEMPERATURE in config:
        sens = await sensor.new_sensor(config[CONF_TEMPERATURE])
        cg.add(var.set_temperature_sensor(sens))
    if CONF_HUMIDITY in config:
        sens = await sensor.new_sensor(config[CONF_HUMIDITY])
        cg.add(var.set_humidity_sensor(sens))
