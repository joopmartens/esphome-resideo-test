/**
  @AUTHOR (c) 2025 Pluimvee (Erik Veer)

  Specifications CHT8305 T/H Sensor
  Oct 2017 rev 1.1 SENSYLINK MIcroelectronics Co. LTD
  https://www.semiee.com/file/Sensylink/Sensylink-CHT8305.pdf

 */

#include "cht8305_sniffer_sensor.h"

namespace esphome {
namespace cht8305_sniffer {

///////////////////////////////////////////////////////////////////////////////////////////////////
static volatile byte device_address;
static volatile byte device_register[256];
static volatile byte device_register_ptr;

static volatile bool i2cIdle = true;  // true if the I2C BUS is idle
static volatile int bitIdx  = 0;      // the bitindex within the frame
static volatile int byteIdx = 0;      // nr of bytes within tis frame
static volatile byte data = 0;        // the byte under construction, which will persited in device_register when acknowledged
static volatile bool writing = true;  // is the master reading or writing to the device
static int PIN_SCL = 5;               // default ESP8266 D1
static int PIN_SDA = 4;               // default ESP8266 D2

///////////////////////////////////////////////////////////////////////////////////////////////////
//// Interrupt handlers
///////////////////////////////////////////////////////////////////////////////////////////////////
// Rising SCL makes us reading the SDA pin
void IRAM_ATTR i2cTriggerOnRaisingSCL() 
{
  if (i2cIdle)    // we didnt get a start signal yet
    return;

	//get the value from SDA
	int sda = digitalRead(PIN_SDA);

	//decide where we are and what to do with incoming data
	int i2cCase = 0;    // data bit

	if (bitIdx == 8)    // we are already at 8 bits, so this is the (N)ACK bit
		i2cCase = 1; 

	if (bitIdx == 7 && byteIdx == 0 ) // first byte is 7bits address, 8th bit is R/W
		i2cCase = 2;

 	bitIdx++; // we found the first bit (out of the switch as its also needed for case 2!)

	switch (i2cCase)
	{
        default:
		case 0: // data bit
            data = (data << 1) | (sda>0?1:0);
	   	    break;//end of case 0 general

		case 1: //(N)ACK
            switch (byteIdx)
            {
            case 0:
                if (sda == 0) // SDA LOW ->  ACK 
                    device_address = data;
                break;
                
            case 1:
                if (writing && sda == 0) {  // if the master is writing, the first byte is the address in the device registers
                    device_register_ptr = data;
                    break;
                }
            // fall thru
            default:
                // it seems that while reading the master signals the slave to stop sending by giving a nack
                // this last byte still needs to be stored in the register
                // remove next line if you want to ignore nack
                if (sda ==0)
                    device_register[device_register_ptr++] = data;
                break;
            }
            byteIdx++;  // next byte
            data = 0;   // reset this data byte
            bitIdx = 0; // start with bit 0
            break;

		case 2:
            writing = (sda == 0);  // if SDA is LOW, the master wants to write 
  		    break;
	}
}

/**
 * This is for recognizing I2C START and STOP
 * This is called when the SDA line is changing
 * It is decided inside the function wheather it is a rising or falling change.
 * If SCL is on High then the falling change is a START and the rising is a STOP.
 * If SCL is LOW, then this is the action to set a data bit, so nothing to do.
 */
void IRAM_ATTR i2cTriggerOnChangeSDA()
{
  if (digitalRead(PIN_SCL) == 0)  // if SCL is low we are still communicating
    return;

	if (digitalRead(PIN_SDA) > 0) //RISING if SDA is HIGH (1) -> STOP 
		i2cIdle = true;
	else //FALLING if SDA is LOW -> START?
	{
		if (i2cIdle) //If we are idle than this is a START
		{
			bitIdx  = 0;
			byteIdx = 0;
            data = 0;
            device_register_ptr = 0;
			i2cIdle = false;
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

    //reset variables
    memset((void *) device_register, 0, sizeof(device_register));
    i2cIdle = true;

    pinMode(PIN_SCL, INPUT_PULLUP);
    pinMode(PIN_SDA, INPUT_PULLUP);

    // Attach interrupts for SDA/SCL pins to your handler functions
    attachInterrupt(PIN_SCL, i2cTriggerOnRaisingSCL, RISING); //trigger for reading data from SDA
    attachInterrupt(PIN_SDA, i2cTriggerOnChangeSDA,  CHANGE); //for I2C START and STOP
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// capture the RAW values when I2C is idle. We do this in the loop() method, so we are
// 1. fast and dont not miss any values
// 2. independend on the update_interval
///////////////////////////////////////////////////////////////////////////////////////////////////
void CHT8305SnifferSensor::loop() 
{
   if (i2cIdle) {
     last_temperature_raw_ = (device_register[0] <<8 | device_register[1]);
     last_humidity_raw_    = (device_register[2] <<8 | device_register[3]);
   }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// This is called by the PollingComponent to update the sensor values.
// It is called every update_interval (default 5000ms).
// we convert the raw values based on the specifications of the CHT8305 sensor.
// However I noticed that the values are not exactly matching the specifications, so I added a configurable offset.
///////////////////////////////////////////////////////////////////////////////////////////////////
void CHT8305SnifferSensor::update() 
{
    float temp = (static_cast<float>(last_temperature_raw_) * 165.0f / 65535.0f) - 40.0f + temperature_offset_;
    float hum = (static_cast<float>(last_humidity_raw_) * 100.0f / 65535.0f) + humidity_offset_;

    if (temperature_sensor_ != nullptr)
        temperature_sensor_->publish_state(temp);
    if (humidity_sensor_ != nullptr)
        humidity_sensor_->publish_state(hum);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

}  // namespace cht8305_sniffer
}  // namespace esphome
