Honeywell Resideo R200C2-A Mod for ESPHome
=========================
Credits for Pluimvee for providing the initial code https://github.com/Pluimvee/Resideo

Custom component for ESPHome to snif traffic between the Resideo firmware and the
- cht8305 chip (I2C) measuring humidity and temperature
- cm1106 chip (RS232) measuring CO2 levels

Usage
-----
I noticed differences between what the Resideo displays on screen and the values received from the cht8305 sensor, converted according the the specs. The sensor should be calibrated during manufacturing so the received values should be correct. However the casing may effect the values and this may have been adjusted by Resideo (Honewell) in the firmware. You can use the offset values using the standard filters of esphome to match the values with what's displayed on screen.

```yaml
external_components:
  - source: github://joopmartens/esphome-resideo
    components: [cht8305_sniffer, cm1106_sniffer]
    refresh: 300s

uart:
  rx_pin: 20  #RX pin used for reading the cm1106 sensor
  baud_rate: 9600
  id: cm1106_uart

sensor:
  - platform: cht8305_sniffer
    sda_pin: 8  #sda pin used for reading the cht8305 sensor
    scl_pin: 9  #scl pin used for reading the cht8305 sensor
    update_interval: 10s
    temperature:
      name: "Temperature"
      filters:
        - calibrate_linear: # adjust temperature if needed
          - 0.00 -> 0.00
          - 26.5 -> 20.0
    humidity:
      name: "Humidity"
      filters:
        - offset: 2.1  # Adjust humidity offset if needed

  - platform: cm1106_sniffer
    uart_id: cm1106_uart
    update_interval: 10s
    name: "CO₂ Sensor"
```

NOTE
-----
In case your ESP device only has 1 UART, be sure to disable the use of the UART by the logger, by setting baudrate to 0. The UART is needed to receive messages from the cm1106 sensor.

``` yaml
# Enable logging
logger:
  level: INFO  
  baud_rate: 0  # Disable default ESP logging on UART (Only required when your ESP device only has 1 UART)
```

Hardware used
--------
- esp32-c3-devkitm-1
- Resideo
