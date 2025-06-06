/**
  @AUTHOR Pluimvee

  Specifications CM1106 CO2-Sensor
  https://en.gassensor.com.cn/Product_files/Specifications/CM1106-C%20Single%20Beam%20NDIR%20CO2%20Sensor%20Module%20Specification.pdf

 */ 
#include "cm1106_sniffer_sensor.h"
#include "esphome/core/log.h"

namespace esphome {
namespace cm1106_sniffer {

static const char *TAG = "cm1106_sniffer";

///////////////////////////////////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////////////////////////////////////
void CM1106SnifferSensor::setup() {
  Serial.begin(9600);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////////////////////////////////////
void CM1106SnifferSensor::loop() {
  uint8_t response[8] = {0};
  int available = Serial.available();
  if (available < 8) 
    return;

  while (available >= 16) {
    Serial.readBytes(response, 8);
    available = Serial.available();
  }

  int matched = 0;
  uint8_t expected_header[3] = {0x16, 0x05, 0x01};
  while (matched < 3) {
    int b = Serial.read();
    if (b == -1) return;
    if (b == expected_header[matched])
      matched++;
    else
      matched = 0;
  }

  int remaining = 8 - 3;
  for (int i = 0; i < 3; ++i)
    response[i] = expected_header[i];
  Serial.readBytes(&response[3], remaining);

  uint8_t crc = 0;
  for (int i = 0; i < 7; ++i)
    crc -= response[i];
  uint16_t ppm = (response[3] << 8) | response[4];
  if (response[7] == crc) {
    cached_ppm_ = ppm;
  } else {
    ESP_LOGW(TAG, "Bad checksum: got 0x%02X, expected 0x%02X", response[7], crc);
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////////////////////////////////////
void CM1106SnifferSensor::update() {
    this->publish_state(cached_ppm_);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

}  // namespace cm1106_sniffer
}  // namespace esphome
