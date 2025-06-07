/**
  @AUTHOR (c) 2025 Pluimvee (Erik Veer)

  Specifications CM1106 CO2-Sensor
  https://en.gassensor.com.cn/Product_files/Specifications/CM1106-C%20Single%20Beam%20NDIR%20CO2%20Sensor%20Module%20Specification.pdf

 */ 
#include "cm1106_sniffer_sensor.h"
#include "esphome/core/log.h"

namespace esphome {
namespace cm1106_sniffer {

static const char *TAG = "cm1106_sniffer";

///////////////////////////////////////////////////////////////////////////////////////////////////
// We use the standard (default) Serial (UART) to receive the data from the CM1106 sensor.
///////////////////////////////////////////////////////////////////////////////////////////////////
void CM1106SnifferSensor::setup() {
  Serial.begin(9600);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// We capture the value in the loop() method, so we are
// 1. fast to not miss any values
// 2. independend on the update_interval
///////////////////////////////////////////////////////////////////////////////////////////////////
void CM1106SnifferSensor::loop() 
{
  uint8_t response[8] = {0};

  // All read responses start with 0x16
  // The payload length for the Co2 message is 0x05
  // The command for the Co2 message is 0x01
  uint8_t expected_header[3] = {0x16, 0x05, 0x01};

  // first check if there are sufficient bytes available
  int available = Serial.available();
  if (available < 8) 
    return;

  // consume old messages
  while (available >= 16) {
    Serial.readBytes(response, 8);
    available = Serial.available();
  }

  // find the start of the message
  int matched = 0;
  while (matched < sizeof(expected_header)) {
      if (Serial.available()) {
        Serial.readBytes(response + matched, 1);
      } else {
        return; // exit if we didnt find the header and uart queue is empty
      }
      if (response[matched] == expected_header[matched]) {
        matched++;
      } else {
        matched = 0; // reset if we don't match
      }
  }
  // if we end up here we have found the header
  available = Serial.available();
  if (available < sizeof(response) - matched) {
    ESP_LOGW(TAG, "Not enough bytes available after header match: %d", available);
    return; // not enough bytes available to read the rest of the message
  }
  matched += Serial.readBytes(&response[matched], sizeof(response) - matched);

  // Checksum: 256-(HEAD+LEN+CMD+DATA)%256
  uint8_t crc = 0;
  for (int i = 0; i < matched; ++i)
    crc -= response[i];

  // We have included the CRC checksum (last byte of the response) so the crc should equal to 0x00
  if (crc != 0x00) {
    ESP_LOGW(TAG, "Bad checksum: got 0x%02X, expected 0x00", crc);
    return; // checksum does not match
  }
  // everything is okay, store the PPM in cached_ppm_ so it can be published
  cached_ppm_ = (response[3] << 8) | response[4];
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Publish the cached PPM value to the frontend. 
///////////////////////////////////////////////////////////////////////////////////////////////////
void CM1106SnifferSensor::update() {
    this->publish_state(cached_ppm_);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

}  // namespace cm1106_sniffer
}  // namespace esphome
