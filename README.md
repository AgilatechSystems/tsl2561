##Node addon for hardware TSL2561 sensor

#####This addon should work on any Linux platform, and has been thoroughly tested on BBB

###Install

```
npm install @agilatech/tsl2561
```
OR
```
git clone https://github.com/Agilatech/tsl2561.git
node-gyp configure build
```

###Usage
#####Load the module and create an instance
```
const addon = require('@agilatech/tsl2561');

// create an instance on the /dev/i2c-1 I2C file at address 0x39
const tsl2561 = new addon.Tsl2561('/dev/i2c-1', 0x39);
```
#####Get basic device info
```
const name = tsl2561.deviceName();  // returns string with name of device
const type = tsl2561.deviceType();  // returns string with type of device
const version = tsl2561.deviceVersion(); // returns this software version
const active = tsl2561.deviceActive(); // true if active, false if inactive
const numVals =  tsl2561.deviceNumValues(); // returns the number of paramters sensed
```
####Get parameter info and values
```
const paramName0 = tsl2561.nameAtIndex(0);
const paramType0 = tsl2561.typeAtIndex(0);
const paramVal0  = tsl2561.valueAtIndexSync(0);
```
####Asynchronous value collection is also available
```
tsl2561.valueAtIndex(0, function(err, val) {
    if (err) {
        console.log(err);
    }
    else {
        console.log(`Asynchronous call return: ${val}`);
    }
});
```

###Operation Notes
The TSL2561 outputs luminosity as the human eye would perceive it. The units are in LUX. The lux is the SI unit of illuminance and luminous emittance, measuring luminous flux per unit area. It is equal to one lumen per square metre. In photometry, this is used as a measure of the intensity, as perceived by the human eye, of light that hits or passes through a surface. It is analogous to the radiometric unit watts per square metre, but with the power at each wavelength weighted according to the luminosity function, a standardized model of human visual brightness perception.

###Dependencies
* node-gyp is needed to compile the addon


###Copyright
Copyright © 2016 Agilatech. All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

