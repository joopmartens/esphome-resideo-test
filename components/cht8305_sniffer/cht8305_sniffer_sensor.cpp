/**
 * Specifications CHT8305 T/H Sensor
 * Oct 2017 rev 1.1 SENSYLINK MIcroelectronics Co. LTD
 * https://www.semiee.com/file/Sensylink/Sensylink-CHT8305.pdf
 */

#include "cht8305_sniffer_sensor.h"
#include <freertos/FreeRTOS.h>
#include <driver/gpio.h>
#include <esp_log.h>
#include <vector>
#include <algorithm>
#include <string.h>

namespace esphome {
namespace cht8305_sniffer {

static const char *TAG = "cht8305_sniffer";

// Global pointer to the class instance to allow ISRs (C functions) to access class members.
static CHT8305SnifferSensor *global_instance = nullptr;

// A NACK will end our sniffing session.
#define I2C_HANDLE_NACK if (gpio_get_level(global_instance->pin_sda_) > 0) { global_instance->i2c_idle_ = true; return; }

// Rising SCL makes us reading the SDA pin. This is our ISR handler.
void IRAM_ATTR i2cTriggerOnRaisingSCL(void *arg) {
    if (global_instance->i2c_idle_) {
        return;
    }

    // Get the value from SDA using the ESP-IDF function.
    int sda_val = gpio_get_level(global_instance->pin_sda_);

    // First byte is 7-bit address, 8th bit is R/W.
    if (global_instance->byte_idx_ == 0 && global_instance->bit_idx_ == 7) {
        global_instance->writing_ = (sda_val == 0); // if SDA is LOW, the master wants to write.
    } else if (global_instance->bit_idx_ != 8) { // data bit.
        global_instance->data_ = (global_instance->data_ << 1) | (sda_val > 0 ? 1 : 0);
    } else { // We are at the ninth (N)ACK bit.
        if (global_instance->byte_idx_ == 0) {  // Slave address byte.
            I2C_HANDLE_NACK; // On NACK we end the conversation as the slave declined.
            global_instance->device_address_ = global_instance->data_;
        } else if (global_instance->byte_idx_ == 1 && global_instance->writing_) { // If the master is writing, the second byte is the register address.
            I2C_HANDLE_NACK; // On NACK we end the conversation as the slave declined the address.
            global_instance->device_register_ptr_ = global_instance->data_;
        } else {
            global_instance->device_register_[global_instance->device_register_ptr_++] = global_instance->data_;
            I2C_HANDLE_NACK; // On NACK we end the conversation AFTER storing the data.
        }
        global_instance->byte_idx_++;
        global_instance->data_ = 0;
        global_instance->bit_idx_ = -1;
    }
    global_instance->bit_idx_++;
}

/**
 * This is for recognizing I2C START and STOP.
 * We must check the state of both SCL and SDA at the moment of the interrupt.
 * If SCL is HIGH, a falling edge on SDA is a START, and a rising edge is a STOP.
 */
void IRAM_ATTR i2cTriggerOnChangeSDA(void *arg) {
    if (gpio_get_level(global_instance->pin_scl_) == 0) {
        return;
    }

    if (gpio_get_level(global_instance->pin_sda_) > 0) { // RISING if SDA is HIGH (1) -> STOP.
        global_instance->i2c_idle_ = true;
    } else { // FALLING if SDA is LOW -> START?
        if (global_instance->i2c_idle_) { // If we are idle, this is a START.
            global_instance->bit_idx_ = 0;
            global_instance->byte_idx_ = 0;
            global_instance->data_ = 0;
            global_instance->i2c_idle_ = false;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// CHT8305SnifferSensor Class Implementation
///////////////////////////////////////////////////////////////////////////////////////////////////

void CHT8305SnifferSensor::setup() {
    global_instance = this;

    this->pin_scl_ = (gpio_num_t)this->scl_pin_;
    this->pin_sda_ = (gpio_num_t)this->sda_pin_;

    // Reset variables.
    memset((void *)this->device_register_, 0, sizeof(this->device_register_));
    this->i2c_idle_ = true;
    this->device_register_ptr_ = 0;

    // Configure GPIOs using ESP-IDF functions.
    gpio_config_t scl_config = {
        .pin_bit_mask = (1ULL << this->pin_scl_),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_POSEDGE // Interrupt on rising edge of SCL.
    };
    gpio_config(&scl_config);

    gpio_config_t sda_config = {
        .pin_bit_mask = (1ULL << this->pin_sda_),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_ANYEDGE // Interrupt on any edge of SDA.
    };
    gpio_config(&sda_config);

    // Install the global GPIO ISR service.
    ESP_ERROR_CHECK(gpio_install_isr_service(0));

    // Add the specific handlers for each pin.
    ESP_ERROR_CHECK(gpio_isr_handler_add(this->pin_scl_, i2cTriggerOnRaisingSCL, nullptr));
    ESP_ERROR_CHECK(gpio_isr_handler_add(this->pin_sda_, i2cTriggerOnChangeSDA, nullptr));

    ESP_LOGI(TAG, "CHT8305 Sniffer initialized on SCL:%d, SDA:%d", (int)this->pin_scl_, (int)this->pin_sda_);
}

void CHT8305SnifferSensor::loop() {
    unsigned long now = millis();
    if (this->i2c_idle_ && (now - this->last_sample_time_ > 100)) {
        this->last_sample_time_ = now;
        uint16_t temp = (uint16_t)this->device_register_[0] << 8 | this->device_register_[1];
        uint16_t humidity = (uint16_t)this->device_register_[2] << 8 | this->device_register_[3];

        this->temperature_raw_.push_back(temp);
        this->humidity_raw_.push_back(humidity);
    }
}

void CHT8305SnifferSensor::update() {
    if (this->temperature_raw_.empty() || this->humidity_raw_.empty()) {
        ESP_LOGW(TAG, "No data available to update sensors.");
        return;
    }
    ESP_LOGD(TAG, "Taking the mean from a window size %d", this->temperature_raw_.size());
    std::sort(this->temperature_raw_.begin(), this->temperature_raw_.end());
    std::sort(this->humidity_raw_.begin(), this->humidity_raw_.end());
    
    uint16_t temp_median = this->temperature_raw_[this->temperature_raw_.size() / 2];
    uint16_t hum_median = this->humidity_raw_[this->humidity_raw_.size() / 2];

    // Convert raw data to actual values
    float temp_value = (static_cast<float>(temp_median) * 165.0f / 65535.0f) - 40.0f;
    float hum_value = (static_cast<float>(hum_median) * 100.0
    
    // Validate ranges before publishing
    if (temp_value >= -20 && temp_value <= 100) {
      float temp = temp_value;
        if (this->temperature_sensor_ != nullptr)
            this->temperature_sensor_->publish_state(temp);
    } else {
        ESP_LOGW(TAG, "Temperature value %.2f°C is out of range, not publishing.", temp_value);
    }

    if (hum_value >= 1 && hum_value <= 100) {
      float hum = hum_value;
        if (this->humidity_sensor_ != nullptr)
            this->humidity_sensor_->publish_state(hum);
    } else {
        ESP_LOGW(TAG, "Humidity value %.2f%% is out of range, not publishing.", hum_value);
    }

    this->temperature_raw_.clear();
    this->humidity_raw_.clear();
}

void CHT8305SnifferSensor::dump_config() {
  LOG_SENSOR(TAG, "cht8305_sniffer:", this->temperature_sensor_);
  LOG_SENSOR(TAG, "cht8305_sniffer:", this->humidity_sensor_);
}

} // namespace cht8305_sniffer
} // namespace esphome
