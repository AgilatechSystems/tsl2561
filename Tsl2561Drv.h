/**
 * \file Tsl2561Drv.h
 *
 *  Created by Scott Erholm on 12/22/2016.
 *  Copyright (c) 2016 Agilatech. All rights reserved.
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef __Tsl2561Drv__
#define __Tsl2561Drv__

#include <iostream>
#include <fstream>
#include <termios.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include "I2CDevice.h"
#include "Device.h"
#include "DataManip.h"

#define TSL2561_DELAY_INTTIME_13MS    (15)
#define TSL2561_DELAY_INTTIME_101MS   (120)
#define TSL2561_DELAY_INTTIME_402MS   (450)

#define TSL2561_VISIBLE 2                   // channel 0 - channel 1
#define TSL2561_INFRARED 1                  // channel 1
#define TSL2561_FULLSPECTRUM 0              // channel 0

// I2C address options
#define TSL2561_ADDR_LOW          (0x29)
#define TSL2561_ADDR_FLOAT        (0x39)    // Default address (pin left floating)
#define TSL2561_ADDR_HIGH         (0x49)

#define TSL2561_COMMAND_BIT       (0x80)    // Must be 1
#define TSL2561_CLEAR_BIT         (0x40)    // Clears any pending interrupt (write 1 to clear)
#define TSL2561_WORD_BIT          (0x20)    // 1 = read/write word (rather than byte)
#define TSL2561_BLOCK_BIT         (0x10)    // 1 = using block read/write

#define TSL2561_CONTROL_POWERON   (0x03)
#define TSL2561_CONTROL_POWEROFF  (0x00)

#define TSL2561_LUX_LUXSCALE      (14)      // Scale by 2^14
#define TSL2561_LUX_RATIOSCALE    (9)       // Scale ratio by 2^9
#define TSL2561_LUX_CHSCALE       (10)      // Scale channel values by 2^10
#define TSL2561_LUX_CHSCALE_TINT0 (0x7517)  // 322/11 * 2^TSL2561_LUX_CHSCALE
#define TSL2561_LUX_CHSCALE_TINT1 (0x0FE7)  // 322/81 * 2^TSL2561_LUX_CHSCALE

// T, FN and CL package values
#define TSL2561_LUX_K1T           (0x0040)  // 0.125 * 2^RATIO_SCALE
#define TSL2561_LUX_B1T           (0x01f2)  // 0.0304 * 2^LUX_SCALE
#define TSL2561_LUX_M1T           (0x01be)  // 0.0272 * 2^LUX_SCALE
#define TSL2561_LUX_K2T           (0x0080)  // 0.250 * 2^RATIO_SCALE
#define TSL2561_LUX_B2T           (0x0214)  // 0.0325 * 2^LUX_SCALE
#define TSL2561_LUX_M2T           (0x02d1)  // 0.0440 * 2^LUX_SCALE
#define TSL2561_LUX_K3T           (0x00c0)  // 0.375 * 2^RATIO_SCALE
#define TSL2561_LUX_B3T           (0x023f)  // 0.0351 * 2^LUX_SCALE
#define TSL2561_LUX_M3T           (0x037b)  // 0.0544 * 2^LUX_SCALE
#define TSL2561_LUX_K4T           (0x0100)  // 0.50 * 2^RATIO_SCALE
#define TSL2561_LUX_B4T           (0x0270)  // 0.0381 * 2^LUX_SCALE
#define TSL2561_LUX_M4T           (0x03fe)  // 0.0624 * 2^LUX_SCALE
#define TSL2561_LUX_K5T           (0x0138)  // 0.61 * 2^RATIO_SCALE
#define TSL2561_LUX_B5T           (0x016f)  // 0.0224 * 2^LUX_SCALE
#define TSL2561_LUX_M5T           (0x01fc)  // 0.0310 * 2^LUX_SCALE
#define TSL2561_LUX_K6T           (0x019a)  // 0.80 * 2^RATIO_SCALE
#define TSL2561_LUX_B6T           (0x00d2)  // 0.0128 * 2^LUX_SCALE
#define TSL2561_LUX_M6T           (0x00fb)  // 0.0153 * 2^LUX_SCALE
#define TSL2561_LUX_K7T           (0x029a)  // 1.3 * 2^RATIO_SCALE
#define TSL2561_LUX_B7T           (0x0018)  // 0.00146 * 2^LUX_SCALE
#define TSL2561_LUX_M7T           (0x0012)  // 0.00112 * 2^LUX_SCALE
#define TSL2561_LUX_K8T           (0x029a)  // 1.3 * 2^RATIO_SCALE
#define TSL2561_LUX_B8T           (0x0000)  // 0.000 * 2^LUX_SCALE
#define TSL2561_LUX_M8T           (0x0000)  // 0.000 * 2^LUX_SCALE

// Auto-gain thresholds
#define TSL2561_AGC_THI_13MS      (4850)    // Max value at Ti 13ms = 5047
#define TSL2561_AGC_TLO_13MS      (100)
#define TSL2561_AGC_THI_101MS     (36000)   // Max value at Ti 101ms = 37177
#define TSL2561_AGC_TLO_101MS     (200)
#define TSL2561_AGC_THI_402MS     (63000)   // Max value at Ti 402ms = 65535
#define TSL2561_AGC_TLO_402MS     (500)

// Clipping thresholds
#define TSL2561_CLIPPING_13MS     (4900)
#define TSL2561_CLIPPING_101MS    (37000)
#define TSL2561_CLIPPING_402MS    (65000)

#define TSL2561_MAX_LUX             21001

enum
{
    TSL2561_REGISTER_CONTROL          = 0x00,
    TSL2561_REGISTER_TIMING           = 0x01,
    TSL2561_REGISTER_THRESHHOLDL_LOW  = 0x02,
    TSL2561_REGISTER_THRESHHOLDL_HIGH = 0x03,
    TSL2561_REGISTER_THRESHHOLDH_LOW  = 0x04,
    TSL2561_REGISTER_THRESHHOLDH_HIGH = 0x05,
    TSL2561_REGISTER_INTERRUPT        = 0x06,
    TSL2561_REGISTER_CRC              = 0x08,
    TSL2561_REGISTER_ID               = 0x0A,
    TSL2561_REGISTER_CHAN0_LOW        = 0x0C,
    TSL2561_REGISTER_CHAN0_HIGH       = 0x0D,
    TSL2561_REGISTER_CHAN1_LOW        = 0x0E,
    TSL2561_REGISTER_CHAN1_HIGH       = 0x0F
};

typedef enum
{
    TSL2561_INTEGRATIONTIME_13MS      = 0x00,    // 13.7ms
    TSL2561_INTEGRATIONTIME_101MS     = 0x01,    // 101ms
    TSL2561_INTEGRATIONTIME_402MS     = 0x02     // 402ms
}
tsl2561IntegrationTime_t;

typedef enum
{
    TSL2561_GAIN_1X                   = 0x00,    // No gain
    TSL2561_GAIN_16X                  = 0x10,    // 16x gain
}
tsl2561Gain_t;

class Tsl2561Drv : public i2cbus::I2CDevice, public Device {

public:
    Tsl2561Drv(std::string devfile, uint32_t addr);
    virtual std::string getValueAtIndex(int index);
    
    static const int NUM_VALUES = 1;
    
protected:
    
    virtual bool initialize();
    virtual std::string readValue0();
    
private:
    
    // Create an array of read functions, so that multiple functions can be easily called
    typedef std::string(Tsl2561Drv::*readValueType)();
    readValueType readFunction[NUM_VALUES] = { &Tsl2561Drv::readValue0 };
    
    void enable(void);
    void disable(void);
    void setIntegrationTime(tsl2561IntegrationTime_t time);
    void setGain(tsl2561Gain_t gain);
    void calcLuminosity ();
    uint32_t calculateLux();
    void getData ();
    uint16_t read16(uint8_t reg);
    
    int gainMult = 0;
    bool autoGain = false;
    tsl2561IntegrationTime_t integrationTime;
    tsl2561Gain_t gain;
    
    uint16_t broadband, ir;

        
};

#endif /* defined(__Tsl2561Drv__) */
