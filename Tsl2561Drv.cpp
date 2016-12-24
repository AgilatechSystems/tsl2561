/*
 * \file Tsl2561Drv.cpp
 *
 *  Created by Scott Erholm on 12/22/2016.
 *  Copyright (c) 2016 Agilatech. All rights reserved.
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "Tsl2561Drv.h"

const std::string Device::name = "TSL2561";
const std::string Device::type = "sensor";

const int Device::numValues = Tsl2561Drv::NUM_VALUES;

const std::string Device::valueNames[numValues] = {"lux"};
const std::string Device::valueTypes[numValues] = {"integer"};

Tsl2561Drv::Tsl2561Drv(std::string devfile, uint32_t addr):i2cbus::I2CDevice(devfile,addr) {

    if (initialize()) {
        this->active = true;
    }
    else {
        std::cerr << name << " did not initialize. " << name << " is inactive" << std::endl;
    }
    
}

std::string Tsl2561Drv::getValueAtIndex(int index) {
    
    if (!this->active) {
        return "none";
    }
    
    if (index < numValues) {
        return (this->*readFunction[index])();
    }
    else {
        return "none";
    }
}

bool Tsl2561Drv::initialize() {
    
    enable();
    
    // Make sure we're actually connected 
    uint8_t x = readRegister(TSL2561_REGISTER_ID);
    if (!(x & 0x0A)) {
        return false;
    }
    
    switch (gainMult) {
        case 1: setGain(TSL2561_GAIN_1X);
            break;
        case 16: setGain(TSL2561_GAIN_16X);
            break;
        default: setGain(TSL2561_GAIN_1X);
    }
    
    // Changing the integration time controls the sensor resolution
    // TSL2561_INTEGRATIONTIME_402ms  // 16-bit data but slowest conversions 
    // TSL2561_INTEGRATIONTIME_13MS   // fast but low resolution 
    // TSL2561_INTEGRATIONTIME_101MS  // medium resolution and speed   
    setIntegrationTime(TSL2561_INTEGRATIONTIME_402MS);
    
    // Start the device in power-down mode at boot
    disable();
    
    return true;
}

std::string Tsl2561Drv::readValue0() {
    
    if (!this->active) {
        return "none";
    }
    
    calcLuminosity();
    
    return DataManip::dataToString((int)calculateLux());
}

void Tsl2561Drv::enable(void) {
    // Enable the device by setting the control bit to 0x03 
    writeRegister(TSL2561_COMMAND_BIT | TSL2561_REGISTER_CONTROL, TSL2561_CONTROL_POWERON);
}

void Tsl2561Drv::disable(void) {
    // Turn the device off to save power 
    writeRegister(TSL2561_COMMAND_BIT | TSL2561_REGISTER_CONTROL, TSL2561_CONTROL_POWEROFF);
}

void Tsl2561Drv::setIntegrationTime(tsl2561IntegrationTime_t time) {
    
    enable();
    
    // Update the timing register 
    writeRegister(TSL2561_COMMAND_BIT | TSL2561_REGISTER_TIMING, time | this->gain);
    
    // Update value placeholders 
    this->integrationTime = time;
    
    // Turn the device off to save power 
    disable();
}

void Tsl2561Drv::setGain(tsl2561Gain_t gain) {

    enable();
    
    // Update the timing register 
    writeRegister(TSL2561_COMMAND_BIT | TSL2561_REGISTER_TIMING, this->integrationTime | gain);
    
    // Update value placeholders 
    this->gain = gain;
    
    // Turn the device off to save power 
    disable();
}

void Tsl2561Drv::calcLuminosity () {
    bool valid = false;
    
    // If Auto gain disabled get a single reading and continue 
    if(!this->autoGain)
    {
        getData ();
        return;
    }
    
    // Read data until we find a valid range 
    bool agcCheck = false;
    
    do {
        uint16_t hi, lo;
        
        // Get the hi/low threshold for the current integration time 
        switch(this->integrationTime)
        {
            case TSL2561_INTEGRATIONTIME_13MS:
                hi = TSL2561_AGC_THI_13MS;
                lo = TSL2561_AGC_TLO_13MS;
                break;
            case TSL2561_INTEGRATIONTIME_101MS:
                hi = TSL2561_AGC_THI_101MS;
                lo = TSL2561_AGC_TLO_101MS;
                break;
            default:
                hi = TSL2561_AGC_THI_402MS;
                lo = TSL2561_AGC_TLO_402MS;
                break;
        }
        
        getData();
        
        // Run an auto-gain check if we haven't already done so ... 
        if (!agcCheck)
        {
            if ((this->broadband < lo) && (this->gain == TSL2561_GAIN_1X))
            {
                // Increase the gain and try again 
                setGain(TSL2561_GAIN_16X);
                // Drop the previous conversion results 
                getData();
                // Set a flag to indicate we've adjusted the gain 
                agcCheck = true;
            }
            else if ((this->broadband > hi) && (this->gain == TSL2561_GAIN_16X))
            {
                // Drop gain to 1x and try again 
                setGain(TSL2561_GAIN_1X);
                // Drop the previous conversion results 
                getData();
                // Set a flag to indicate we've adjusted the gain 
                agcCheck = true;
            }
            else
            {
                // Nothing to look at here, keep moving ....
                // Reading is either valid, or we're already at the chips limits
                valid = true;
            }
        }
        else
        {
            // If we've already adjusted the gain once, just return the new results.
            // This avoids endless loops where a value is at one extreme pre-gain,
            // and the the other extreme post-gain
            valid = true;
        }
    } while (!valid);
}

uint32_t Tsl2561Drv::calculateLux() {
    uint32_t chScale;
    uint32_t channel1;
    uint32_t channel0;
    
    // Make sure the sensor isn't saturated! 
    uint16_t clipThreshold;
    switch (this->integrationTime)
    {
        case TSL2561_INTEGRATIONTIME_13MS:
            clipThreshold = TSL2561_CLIPPING_13MS;
            break;
        case TSL2561_INTEGRATIONTIME_101MS:
            clipThreshold = TSL2561_CLIPPING_101MS;
            break;
        default:
            clipThreshold = TSL2561_CLIPPING_402MS;
            break;
    }
    
    // Return max value lux if the sensor is saturated 
    if ((this->broadband > clipThreshold) || (this->ir > clipThreshold))
    {
        return TSL2561_MAX_LUX;
    }
    
    // Get the correct scale depending on the intergration time 
    switch (this->integrationTime)
    {
        case TSL2561_INTEGRATIONTIME_13MS:
            chScale = TSL2561_LUX_CHSCALE_TINT0;
            break;
        case TSL2561_INTEGRATIONTIME_101MS:
            chScale = TSL2561_LUX_CHSCALE_TINT1;
            break;
        default: // No scaling ... integration time = 402ms 
            chScale = (1 << TSL2561_LUX_CHSCALE);
            break;
    }
    
    // Scale for gain (1x or 16x) 
    if (!this->gain) chScale = chScale << 4;
    
    // Scale the channel values 
    channel0 = (this->broadband * chScale) >> TSL2561_LUX_CHSCALE;
    channel1 = (this->ir * chScale) >> TSL2561_LUX_CHSCALE;
    
    // Find the ratio of the channel values (Channel1/Channel0) 
    uint32_t ratio1 = 0;
    if (channel0 != 0) ratio1 = (channel1 << (TSL2561_LUX_RATIOSCALE+1)) / channel0;
    
    // round the ratio value 
    uint32_t ratio = (ratio1 + 1) >> 1;
    
    uint32_t b, m;
    
    if (ratio <= TSL2561_LUX_K1T)
    {b=TSL2561_LUX_B1T; m=TSL2561_LUX_M1T;}
    else if (ratio <= TSL2561_LUX_K2T)
    {b=TSL2561_LUX_B2T; m=TSL2561_LUX_M2T;}
    else if (ratio <= TSL2561_LUX_K3T)
    {b=TSL2561_LUX_B3T; m=TSL2561_LUX_M3T;}
    else if (ratio <= TSL2561_LUX_K4T)
    {b=TSL2561_LUX_B4T; m=TSL2561_LUX_M4T;}
    else if (ratio <= TSL2561_LUX_K5T)
    {b=TSL2561_LUX_B5T; m=TSL2561_LUX_M5T;}
    else if (ratio <= TSL2561_LUX_K6T)
    {b=TSL2561_LUX_B6T; m=TSL2561_LUX_M6T;}
    else if (ratio <= TSL2561_LUX_K7T)
    {b=TSL2561_LUX_B7T; m=TSL2561_LUX_M7T;}
    else if (ratio > TSL2561_LUX_K8T)
    {b=TSL2561_LUX_B8T; m=TSL2561_LUX_M8T;}
    
    uint32_t temp;
    
    // Do not allow negative lux value 
    if ((channel1 * m) > (channel0 * b)) {
        temp = 0;
    }
    else {
        temp = ((channel0 * b) - (channel1 * m));
    }
    
    // Round lsb (2^(LUX_SCALE-1)) 
    temp += (1 << (TSL2561_LUX_LUXSCALE-1));
    
    // Strip off fractional portion 
    uint32_t lux = temp >> TSL2561_LUX_LUXSCALE;
    
    return lux;
}

void Tsl2561Drv::getData () {

    enable();
    
    // Wait x ms for ADC to complete 
    switch (this->integrationTime)
    {
        case TSL2561_INTEGRATIONTIME_13MS:
            usleep(TSL2561_DELAY_INTTIME_13MS * 1000);
            break;
        case TSL2561_INTEGRATIONTIME_101MS:
            usleep(TSL2561_DELAY_INTTIME_101MS * 1000); // KTOWN: Was 102ms
            break;
        default:
            usleep(TSL2561_DELAY_INTTIME_402MS * 1000); // KTOWN: Was 403ms
            break;
    }
    
    // Reads a two byte value from channel 0 (visible + infrared) 
    this->broadband = read16(TSL2561_COMMAND_BIT | TSL2561_WORD_BIT | TSL2561_REGISTER_CHAN0_LOW);
    
    // Reads a two byte value from channel 1 (infrared) 
    this->ir = read16(TSL2561_COMMAND_BIT | TSL2561_WORD_BIT | TSL2561_REGISTER_CHAN1_LOW);
    
    // Turn the device off to save power 
    disable();
}

uint16_t Tsl2561Drv::read16(uint8_t reg) {
    uint16_t h; uint16_t l;
    
    l = readRegister(reg);
    h = readRegister(reg+1);
    
    return ((short)h<<8)|(short)l;
}



