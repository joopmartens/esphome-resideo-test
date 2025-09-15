#pragma once

#include "esphome/components/sensor/sensor.h"
#include "esphome/components/uart/uart.h"
#include "esphome/core/component.h"
#include "esphome/core/helpers.h"

namespace esphome {
namespace cm1106_sniffer {

class CM1106Sniffer : public sensor::Sensor, public PollingComponent {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;
  void update() override;

  void set_uart_parent(uart::UARTComponent *uart_parent) { this->uart_component_ = uart_parent; }

 protected:
  void handle_byte(uint8_t byte);
  void reset_buffer_();
  
  uart::UARTComponent *uart_component_{nullptr};
  uint8_t buffer_[9];
  uint8_t buffer_pos_{0};
  std::vector<uint16_t> co2_raw_;
  //uint16_t co2_value_ = 0;
  //sensor::Sensor *co2_sensor_ = nullptr;
  //bool should_update_ = false;
  bool frame_ready_ = false;
};

}  // namespace cm1106_sniffer
}  // namespace esphome