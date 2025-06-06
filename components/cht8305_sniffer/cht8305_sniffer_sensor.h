#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"

namespace esphome {
namespace cht8305_sniffer {

class CHT8305SnifferSensor : public PollingComponent {
 public:
  CHT8305SnifferSensor() : PollingComponent(5000) {}
  void set_temperature_sensor(sensor::Sensor *sensor) { temperature_sensor_ = sensor; }
  void set_humidity_sensor(sensor::Sensor *sensor) { humidity_sensor_ = sensor; }
  void set_sda_pin(int pin) { sda_pin_ = pin; }
  void set_scl_pin(int pin) { scl_pin_ = pin; }
  void set_temperature_offset(float offset) { temperature_offset_ = offset; }
  void set_humidity_offset(float offset) { humidity_offset_ = offset; }

  void setup() override;
  void loop() override;
  void update() override;

 protected:
  sensor::Sensor *temperature_sensor_{nullptr};
  sensor::Sensor *humidity_sensor_{nullptr};
  int sda_pin_{4}; // Default ESP8266 D2
  int scl_pin_{5}; // Default ESP8266 D1
  float temperature_offset_{0.0f};
  float humidity_offset_{0.0f};
  // Raw register storage, mimicking your sniffer state (minimalistic example)
  uint16_t last_temperature_raw_{0};
  uint16_t last_humidity_raw_{0};
};

}  // namespace cht8305_sniffer
}  // namespace esphome
