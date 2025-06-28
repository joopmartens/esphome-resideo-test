import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import (
    DEVICE_CLASS_CARBON_DIOXIDE,
    UNIT_PARTS_PER_MILLION,
    ICON_MOLECULE_CO2,
)

cm1106_ns = cg.esphome_ns.namespace('cm1106_sniffer')
CM1106Sniffer = cm1106_ns.class_('CM1106SnifferSensor', sensor.Sensor, cg.PollingComponent)

CONFIG_SCHEMA = sensor.sensor_schema(
    unit_of_measurement=UNIT_PARTS_PER_MILLION,
    icon=ICON_MOLECULE_CO2,
    accuracy_decimals=0,
    device_class=DEVICE_CLASS_CARBON_DIOXIDE,
    state_class='measurement',
).extend({
    cv.GenerateID(): cv.declare_id(CM1106Sniffer),
}).extend(cv.polling_component_schema("5s"))

async def to_code(config):
    var = await sensor.new_sensor(config)
    await cg.register_component(var, config)
