import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor, uart
from esphome.const import (
    CONF_ID,
    CONF_UPDATE_INTERVAL,
    UNIT_PARTS_PER_MILLION,
    ICON_MOLECULE_CO2,
    STATE_CLASS_MEASUREMENT,
    DEVICE_CLASS_CARBON_DIOXIDE,
)

# Define the namespace for the C++ component
cm1106_sniffer_ns = cg.esphome_ns.namespace("cm1106_sniffer")

# Explicitly declare the C++ class from the header file
CM1106Sniffer = cm1106_sniffer_ns.class_("CM1106Sniffer", sensor.Sensor, cg.PollingComponent, uart.UARTDevice)

# Define the configuration schema for the component
CONFIG_SCHEMA = (
    sensor.sensor_schema(
        unit_of_measurement=UNIT_PARTS_PER_MILLION,
        icon=ICON_MOLECULE_CO2,
        accuracy_decimals=0,
        device_class=DEVICE_CLASS_CARBON_DIOXIDE,
        state_class=STATE_CLASS_MEASUREMENT,
    )
    .extend(
        {
            cv.GenerateID(): cv.declare_id(CM1106Sniffer),
        }
    )
    .extend(cv.polling_component_schema("10s"))
    .extend(uart.UART_DEVICE_SCHEMA)
)


def to_yaml(config):
    # This function is not strictly needed for this simple sensor, but is
    # good practice for more complex components to handle YAML output.
    pass


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    # Use a sensor schema
    await sensor.register_sensor(var, config)
    # Get the UART bus ID from the configuration
    await uart.register_uart_device(var, config)