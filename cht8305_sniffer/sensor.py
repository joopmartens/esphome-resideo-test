import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import (
    CONF_ID,
    CONF_TEMPERATURE,
    CONF_HUMIDITY,
    CONF_NAME,
)

CONF_SDA_PIN = "sda_pin"
CONF_SCL_PIN = "scl_pin"

cht8305_ns = cg.esphome_ns.namespace("cht8305_sniffer")
CHT8305Sniffer = cht8305_ns.class_("CHT8305SnifferSensor", cg.Component)

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(CHT8305Sniffer),
    cv.Optional(CONF_SDA_PIN, default=4): cv.int_,   # D2 on ESP8266
    cv.Optional(CONF_SCL_PIN, default=5): cv.int_,   # D1 on ESP8266
    cv.Optional(CONF_TEMPERATURE, default={}): sensor.sensor_schema(),
    cv.Optional(CONF_HUMIDITY, default={}): sensor.sensor_schema(),
}).extend(cv.COMPONENT_SCHEMA)

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
