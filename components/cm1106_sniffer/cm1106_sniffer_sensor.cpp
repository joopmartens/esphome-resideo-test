/**

  Specifications CM1106 CO2-Sensor
  https://en.gassensor.com.cn/Product_files/Specifications/CM1106-C%20Single%20Beam%20NDIR%20CO2%20Sensor%20Module%20Specification.pdf

 */ 

#include "cm1106_sniffer_sensor.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"
#include "esphome/components/uart/uart.h"

namespace esphome {
namespace cm1106_sniffer {

static const char *const TAG = "cm1106_sniffer";

void CM1106Sniffer::setup() {
  ESP_LOGCONFIG(TAG, "Setting up CM1106 Sniffer Sensor...");
  if (this->uart_component_ == nullptr) {
    ESP_LOGE(TAG, "UART component not set!");
    return;
  }
  this->uart_component_->flush();
}

void CM1106Sniffer::loop() {
  if (this->uart_component_ == nullptr) {
    return;
  }
  // Only process one frame if an update is due
  //if (!this->should_update_) {
  //  return;
  //}
  while (this->uart_component_->available()) {
    uint8_t byte;
    this->uart_component_->read_byte(&byte);
    this->handle_byte(byte);
    // After handling one full frame, stop processing
    if (this->frame_ready_) {
      //this->should_update_ = false;
      this->frame_ready_ = false;
      return;
    }
  }
}

void CM1106Sniffer::handle_byte(uint8_t byte) {
  if (this->buffer_pos_ == 0 && byte != 0x16) {
    return;
  }
  
  this->buffer_[this->buffer_pos_++] = byte;

  if (this->buffer_pos_ < 9) {
    return;
  }

  // Log the received frame for debugging
  ESP_LOGD(TAG, "Received frame: %s", format_hex_pretty(this->buffer_, 9).c_str());

  if (this->buffer_[0] != 0x16) {
    ESP_LOGW(TAG, "Invalid start byte: 0x%02X", this->buffer_[0]);
    this->reset_buffer_();
    return;
  }

  uint8_t checksum = 0;
  // Checksum is the two's complement of the sum of bytes 1 through 7.
  for (int i = 1; i < 8; ++i) {
    checksum += this->buffer_[i];
  }
  checksum = 0xFF - checksum + 1;
  
  if (this->buffer_[8] != checksum) {
    ESP_LOGW(TAG, "Checksum mismatch: calculated 0x%02X, received 0x%02X", checksum, this->buffer_[8]);
    this->reset_buffer_();
    return;
  }

  uint16_t co2_raw_value = (uint16_t) this->buffer_[3] << 8 | this->buffer_[4];
  this->co2_raw_.push_back(co2_raw_value);

  
  this->reset_buffer_();
  this->frame_ready_ = true; // Signal that a new frame has been processed
}

void CM1106Sniffer::dump_config() {
  //ESP_LOGCONFIG(TAG, "cm1106_sniffer:");
  LOG_SENSOR(TAG, "cm1106_sniffer:", this);
}

void CM1106Sniffer::update() {
  //this->should_update_ = true;
  //this->loop();
  if (this->co2_raw_.empty()) {
      ESP_LOGW(TAG, "No data available to update sensor.");
      return;
  }
  ESP_LOGD(TAG, "Taking the mean from a window size %d", this->co2_raw_.size());
  std::sort(this->co2_raw_.begin(), this->co2_raw_.end());

  uint16_t co2_median = this->co2_raw_[this->co2_raw_.size() / 2];

  // Validate CO2 value range before storing
  if (co2_median >= 350 && co2_median <= 5000) {
       this->co2_value_ = co2_median;
       this->publish_state(this->co2_value_);
       this->co2_value_ = 0;
      ESP_LOGD(TAG, "CO2 value: %d ppm", co2_median);
  } else {
    ESP_LOGW(TAG, "Received CO2 value %d ppm is outside the valid range (350-5000), not publishing.", co2_median);
  } 
  

  //this->publish_state(this->co2_value_);
  //this->co2_value_ = 0;
  this->co2_raw_.clear();
}

void CM1106Sniffer::reset_buffer_() {
  this->buffer_pos_ = 0;
}

}  // namespace cm1106_sniffer
}  // namespace esphome