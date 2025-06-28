/**
  @AUTHOR (c) 2025 Pluimvee (Erik Veer)

 */
#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"

namespace esphome {
namespace cm1106_sniffer {

class CM1106SnifferSensor : public sensor::Sensor, public PollingComponent {
public:
  CM1106SnifferSensor() : PollingComponent(5000) {}
  void setup() override;
  void loop() override;
  void update() override;
  void dump_config() override {
    ESP_LOGCONFIG("Resideo", "CM1106 Sniffer (C)Pluimvee");
  };
protected:
  uint16_t cached_ppm_{0};
};

}  // namespace cm1106_sniffer
}  // namespace esphome
