/**
  @AUTHOR (c) 2025 Pluimvee (Erik Veer)

  Specifications CHT8305 T/H Sensor
  Oct 2017 rev 1.1 SENSYLINK MIcroelectronics Co. LTD
  https://www.semiee.com/file/Sensylink/Sensylink-CHT8305.pdf

 */

#include "cht8305_sniffer_sensor.h"

namespace esphome
{
namespace cht8305_sniffer
{

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    static volatile byte device_address;
    static volatile byte device_register[256];
    static volatile byte device_register_ptr;

    static volatile bool i2cIdle = true; // true if the I2C BUS is idle
    static volatile int bitIdx = 0;      // the bitindex within the frame
    static volatile int byteIdx = 0;     // nr of bytes within tis frame
    static volatile byte data = 0;       // the byte under construction, which will persited in device_register when acknowledged
    static volatile bool writing = true; // is the master reading or writing to the device
    static int PIN_SCL = 5;              // default ESP8266 D1
    static int PIN_SDA = 4;              // default ESP8266 D2

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    //// Interrupt handlers
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    // A NACK will end our sniffing session
    // this may happen on first (slave) byte, by which the slave donts want to communicate (not ready)
    // or on the second byte (register address) when the master is writing and the slave does not want to accept the register address
    // or on the last byte when the master is reading and does not want more data.
    #define I2C_HANDLE_NACK if (sda > 0) { i2cIdle = true; return; }
    
    // Rising SCL makes us reading the SDA pin
    void IRAM_ATTR i2cTriggerOnRaisingSCL()
    {
        if (i2cIdle) // we didnt get a start signal yet or we stopped listening
            return;

        // get the value from SDA
        int sda = digitalRead(PIN_SDA);

        // first byte is 7bits address, 8th bit is R/W
        if (byteIdx == 0 && bitIdx == 7)
            writing = (sda == 0); // if SDA is LOW, the master wants to write
        
        else if (bitIdx != 8) // data bit
            data = (data << 1) | (sda > 0 ? 1 : 0);

        else // we are at the ninth (N)ACK bit
        {
            if (byteIdx == 0)   //slave address byte
            {
                I2C_HANDLE_NACK; // On NACK we end the conversation as the slave declined end a restart (Sr) is expected
                device_address = data;
            }
            else if (byteIdx == 1 && writing) // if the master is writing, the second byte is the register address
            {   
                I2C_HANDLE_NACK; // On NACK we end the conversation as the slave declined the address, so we dont know which address will be used
                // its safe to just store the data in the ptr as it will never reach beyond the 256 byte limit
                device_register_ptr = data;
            }        
            else
            {   // we always store the data, even on a NACK (which indicates end of reading)
                device_register[device_register_ptr++] = data;
                I2C_HANDLE_NACK; // On NACK we end the conversation AFTER storing the data. 
            }
            byteIdx++;   // go to the next byte
            data = 0;    // reset this data byte value
            bitIdx = -1; // and restart with bit 0 (set to -1 as we will be incrementing it at the end of this function)
        }
        // go to the next bit
        bitIdx++; 
    }

    /**
     * This is for recognizing I2C START and STOP
     * This is called when the SDA line is changing
     * It is decided inside the function wheather it is a rising or falling change.
     * If SCL is HIGH then the falling change is a START and the rising is a STOP.
     * If SCL is LOW, then this event is an action to set a data bit, so nothing to do.
     */
    void IRAM_ATTR i2cTriggerOnChangeSDA()
    {
        if (digitalRead(PIN_SCL) == 0) // if SCL is low we are still communicating
            return;

        if (digitalRead(PIN_SDA) > 0) // RISING if SDA is HIGH (1) -> STOP
            i2cIdle = true;
        else // FALLING if SDA is LOW -> START?
        {
            if (i2cIdle) // If we are idle than this is a START
            {
                bitIdx = 0;
                byteIdx = 0;
                data = 0;
                i2cIdle = false;
                // we dont reset the register_ptr as it may have been set to prepare for reading!!!
            }
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    // We set the SCL and SDA pins to the values given in the configuration
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CHT8305SnifferSensor::setup()
    {
        // Use configurable pins!
        PIN_SCL = scl_pin_;
        PIN_SDA = sda_pin_;

        // reset variables
        memset((void *)device_register, 0, sizeof(device_register));
        i2cIdle = true;
        device_register_ptr = 0; // the pointer will be initially set to 0

        pinMode(PIN_SCL, INPUT_PULLUP);
        pinMode(PIN_SDA, INPUT_PULLUP);

        // Attach interrupts for SDA/SCL pins to your handler functions
        attachInterrupt(PIN_SCL, i2cTriggerOnRaisingSCL, RISING); // trigger for reading data from SDA
        attachInterrupt(PIN_SDA, i2cTriggerOnChangeSDA, CHANGE);  // for I2C START and STOP
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    // capture the RAW values when I2C is idle. We do this in the loop() method, so we are
    // 1. fast and dont not miss any values
    // 2. independend on the update_interval
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    unsigned long last_sample_time = 0;

    void CHT8305SnifferSensor::loop()
    {
        unsigned long now = millis();
        if (i2cIdle && (now - last_sample_time > 100))
        {
            last_sample_time = now; // update the last sample time
            // copy into local variables as quick as possible
            uint16_t temp = device_register[0] << 8 | device_register[1];
            uint16_t humidity = device_register[2] << 8 | device_register[3];

            // now store the values in the raw data buffers
            temperature_raw_.push_back(temp);
            humidity_raw_.push_back(humidity);
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    // This is called by the PollingComponent to update the sensor values.
    // It is called every update_interval (default 5000ms).
    // we convert the raw values based on the specifications of the CHT8305 sensor.
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CHT8305SnifferSensor::update()
    {
        if (temperature_raw_.empty() || humidity_raw_.empty())
        {
            ESP_LOGW("cht8305_sniffer", "No data available to update sensors.");
            return;
        }
        ESP_LOGD("cht8305_sniffer", "Taking the mean from a window size %d", temperature_raw_.size());
        // sort the raw data to calculate the median
        std::sort(temperature_raw_.begin(), temperature_raw_.end());
        std::sort(humidity_raw_.begin(), humidity_raw_.end());
        
        // Calculate the median of the raw values
        uint16_t temp_median = temperature_raw_[temperature_raw_.size() / 2];
        uint16_t hum_median = humidity_raw_[humidity_raw_.size() / 2];

        float temp = (static_cast<float>(temp_median)* 165.0f / 65535.0f) - 40.0f;
        float hum = (static_cast<float>(hum_median) * 100.0f / 65535.0f);

        // Clear the raw data after processing
        temperature_raw_.clear();
        humidity_raw_.clear();

        if (temperature_sensor_ != nullptr)
            temperature_sensor_->publish_state(temp);
        if (humidity_sensor_ != nullptr)
            humidity_sensor_->publish_state(hum);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace cht8305_sniffer
} // namespace esphome
