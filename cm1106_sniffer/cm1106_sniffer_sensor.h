#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"

namespace esphome {
namespace cm1106_sniffer {

class CM1106SnifferSensor : public sensor::Sensor, public Component {
 public:
  void setup() override;
  void loop() override;

 protected:
  uint16_t cached_ppm_{0};
  void read_co2_uart_();
};

}  // namespace cm1106_sniffer
}  // namespace esphome
