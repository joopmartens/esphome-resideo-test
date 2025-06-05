Resideo ESPHome sniffers
=========================

Custom component for ESPHome to snif traffic between the Resideo firmware and the
- I2C traffic with the cht8305 chip measuring humidity and temperature
- RS232 traffice with the cm1106 chip measuring CO2 levels

Usage
-----

Requires ESPHome v2022.3.0 or newer.

```yaml
external_components:
  - source: github://Pluimvee/esphome-resideo

modbus_bridge:
```


Hardware used
--------
- ESP 12F (1.10 EUR)
- Resideo


