Honeywell Resideo R200C2-A Mod for ESPHome
=========================
MQTT VERSION HERE https://github.com/Pluimvee/Resideo

Custom component for ESPHome to snif traffic between the Resideo firmware and the
- cht8305 chip (I2C) measuring humidity and temperature
- cm1106 chip (RS232) measuring CO2 levels

Usage
-----
I noticed differences between what the Resideo displays on screen and the values received from the cht8305 sensor, converted according the the specs. The sensor should is calibrated during manufacturing so the received values should be correct. However the casing may effect the values and this may have been adjusted by Resideo (Honewell) in the firmware. You can use the offset values using the standard filters of esphome to match the values with what's displayed on screen.

```yaml
external_components:
  - source: github://Pluimvee/esphome-resideo
    components: [cht8305_sniffer, cm1106_sniffer]

sensor:
  - platform: cht8305_sniffer
    temperature:
      name: "Temperature"
      filters:
        - offset: -1.4  # Adjust temperature offset if needed
    humidity:
      name: "Humidity"
      filters:
        - offset: 2.1  # Adjust humidity offset if needed

  - platform: cm1106_sniffer
    # update_interval: 10s default 5s
    name: "CO2 Level"
```

NOTE
-----
Be sure to disable the use of the UART by the logger, by setting baudrate to 0. The UART is needed to receive messages from the cm1106 sensor

``` yaml
# Enable logging
logger:
  level: DEBUG  
  baud_rate: 0  # Disable logging on UART (as we need the UART)
```

Hardware used
--------
- ESP 12F (1.10 EUR)
- Resideo


