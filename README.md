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
    refresh: 1min
    components: [cht8305_sniffer, cm1106_sniffer]

sensor:
  - platform: cht8305_sniffer
    temperature:
      name: "Temperature"
      offset: -1.4  # Adjust temperature offset if needed
    humidity:
      name: "Humidity"
      offset: -0.1  # Adjust humidity offset if needed

  - platform: cm1106_sniffer
    # update_interfval: 10s
    name: "CO2 Concentration"
```


Hardware used
--------
- ESP 12F (1.10 EUR)
- Resideo


