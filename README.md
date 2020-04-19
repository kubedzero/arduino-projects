# ESP8266 Monitoring Code

### OTA Update

* using http://arduino.esp8266.com/Arduino/versions/2.0.0/doc/ota_updates/ota_updates.html#web-browser as the base of the sketch, will allow over the air updates without having to plug into the computer. 
  * Base sketch in the ESP8266HTTPUpdateServer folder of Examples, using the SecureWebUpdater which requires a username and password to allow update
  * Sketch mentions a curl command that can be used to update `curl -u username:password -F "image=@firmware.bin" esp8266-webupdate.local/firmware` 
  * http://arduino.esp8266.com/Arduino/versions/2.0.0/doc/ota_updates/ota_updates.html#implementation-overview provides info on how to integrate it with your normal sketch
  * Compile log shows towards the end `Creating BIN file "/var/folders/gt/b5zkfzhj0z74_q1z2ngthjt00000gn/T/arduino_build_133328/otaUpdate.ino.bin" using...` so I can use CMD + G to go to that location in the web browser http://ipaddress/firmware or I can run the curl command `curl -u myUsername:myPassword -F "image=@/var/folders/gt/b5zkfzhj0z74_q1z2ngthjt00000gn/T/arduino_build_133328/otaUpdate.ino.bin" ip.address/firmware` 
  * The output location of the BIN doesn't seem to change between recompiles
* https://arduino.stackexchange.com/questions/40411/hiding-wlan-password-when-pushing-to-github suggests a way to store Wifi passwords and stuff outside the normal file. 
* Git ignore all files with same name https://stackoverflow.com/questions/8981472/ignore-all-files-with-the-same-name-in-git
* WiFi reconnect method idea https://www.esp8266.com/viewtopic.php?f=32&t=8286
* Currently, the ESPs are all also broadcasting a WiFi network. https://arduino-esp8266.readthedocs.io/en/latest/esp8266wifi/soft-access-point-class.html#softapdisconnect seems to describe this as a SoftAP because it's using the same hardware as the WiFi connection. Apparently it can be turned off with `WiFi.softAPdisconnect(true)`. Alternatively, https://arduino-esp8266.readthedocs.io/en/latest/esp8266wifi/readme.html notes we can just initialize Wifi with a different mode, maybe STA instead of `WiFi.mode(WIFI_AP_STA);` https://arduino-esp8266.readthedocs.io/en/latest/esp8266wifi/generic-class.html confirms this, notes that `WiFi.mode(m)`: set mode to `WIFI_AP`, `WIFI_STA`, `WIFI_AP_STA` or `WIFI_OFF



### BME280 Integration

* BME280 is the ENVIRONMENT sensor by Bosch, BMP280 is the PRESSURE sensor. 
  * BME provides humidity, pressure, and temperature
  * BMP provides pressure and temperature
* actively updated Arduino libraries
  * Sparkfun https://github.com/sparkfun/SparkFun_BME280_Arduino_Library
  * Adafruit https://github.com/adafruit/Adafruit_BME280_Library
    * requires companion library https://github.com/adafruit/Adafruit_Sensor which also supports DHT
    * Separate library for BMP280 
    * SDA/SCL default to pins 4 & 5 but any two pins can be assigned as SDA/SCL using Wire.begin(SDA,SCL)
  * finiteSpace/Tyler Glenn https://github.com/finitespace/BME280
  * BlueDot https://github.com/BlueDot-Arduino/BlueDot_BME280
* Currently plugged into D1-> SCL and D2-> SDA
* 2020/03/04 Update: Trying to update from the 1.1.0 version of the BME280 library to the 2.0.1 version, which notes a breaking change of dumbing down the sensor detection. Currently in 1.1.0 my Trace logs show `Wire SensorID was: 0x96` whereas I think it's expecting 0x77 or 0x76. After updating the library though the trace shows the `Wire SensorID was: 0x255`. I had to update the signature to `bme280.begin(0x76, &Wire)` to get it working, and then it worked just as it used to. 
* https://playground.arduino.cc/Main/I2cScanner/ helped me scan through the I2C addresses to find what was connected, and helped identify that 0x96 was not the correct address

### BMP280 Integration

* libraries
  * similar to BME280 but Adafruit has a separate BMP280 library
* Adafruit setup reference https://learn.adafruit.com/adafruit-bmp280-barometric-pressure-plus-temperature-sensor-breakout/arduino-test

* Did not find the sensor even with the Adafruit bmp280test on D1->SCL (pin 5) and D2->SDA (pin4) as recommended.
* https://github.com/adafruit/Adafruit_BMP280_Library/blob/master/Adafruit_BMP280.h#L156 showed me that I can call `bmp.begin` with custom overridden BMP280 address and chipID. Since Adafruit has their own version of the BMP280 I thought maybe one of these values was different. 
* `bmp.begin(0x76,0x58)` replaced `bmp.begin()` and then I was able to connect. https://github.com/sparkfun/SparkFun_BME280_Arduino_Library/issues/21 gave me the hint
* chipID of a BMP280 should be 0x58 as per the Bosch BMP280 datasheet. The default I2C address was 0x76 which I think was the issue
* Arduino recommendation for string representation https://www.arduino.cc/reference/en/language/variables/data-types/stringobject/ to avoid the compiler warning "deprecated conversion from string constant to 'char*'"
  * I can declare mutable strings with `String varName = "someString"` and then `varName = "newString"` and can compare string with Boolean output with `varname.equals("anotherString")`
* The debug level of the underlying ESP8266 firmware can be changed in the Boards dropdown by selecting the Debug port and Debug level
* Set the SSL support to basic to save space, since we're not using SSL for this
* Trying a new TaskScheduler library https://github.com/arkhipenko/TaskScheduler instead of the the old cloned-to-github-by-random-people SimpleTimer library written by Marcello Romani https://github.com/zomerfeld/SimpleTimerArduino  https://playground.arduino.cc/Code/SimpleTimer/
  * Chrono https://github.com/SofaPirate/Chrono also seems to be an option
* Using a logging library https://github.com/thijse/Arduino-Log which will let me easily enable and disable different log levels going out to the Serial interface. For the most part I don't want to see every sensor update, but I do want to see whenever the device is accessed
* https://stackoverflow.com/questions/13294067/how-to-convert-string-to-char-array-in-c copying a string into a char array since the Logging library I chose doesn't support Strings https://github.com/thijse/Arduino-Log/blob/master/examples/Log/Log.ino#L61

### PMS7003 Integration

* No Adafruit library but there is https://github.com/fu-hsi/pms. According to https://learn.adafruit.com/pm25-air-quality-sensor/arduino-code though there doesn't really need to be a library, it's just softwareserial and whatever's returned from that
* Used to be set up with the library mentioned above. Pinout from the PMS7003 was
  * PMS -> Arduino (reference direction)
  * nc (red) -> not connected
  * nc (black) -> not connected
  * rat (orange) -> not connected
  * tx (green) -> D6
  * rx (blue) -> D7 BUT not actually needed to function since we're not talking to the PMS7003
  * set (white) -> not connected
  * gnd (red) -> GND
  * vcc (purple) -> 5v or 3.3v works. HOWEVER the datasheet says "DC 5V power supply is needed because the FAN should be driven by 5V. But the high level of data pin is 3.3V. Level conversion unit should be used if the power of host MCU is 5V."
* Refs
  * https://kamami.com/gas-sensors/564008-plantower-pms7003-laser-dust-sensor.html
  * https://www.espruino.com/PMS7003
  * https://aqicn.org/sensor/pms5003-7003/
  * https://learn.adafruit.com/pm25-air-quality-sensor/arduino-code
  * https://github.com/fu-hsi/pms



### UV Sensor Integration

* Bought a VEML6075 sensor from AliExpress https://www.aliexpress.com/item/32797545230.html?spm=a2g0s.9042311.0.0.5b784c4dAI9lSe for about 4 dollars. It measures UVA and UVB intensity and also calculates the UV index  approximation
* I was able to use the SparkFun VEML6075 library while the sensor's SCL pin was connected to the NodeMCU D1 pin and the SDA pin was connected to the NodeMCU D2 pin, and then two other connections for 3.3v power (5v apparently also works) and ground. 
* I was also able to use the I2C scanner https://playground.arduino.cc/Main/I2cScanner/ to confirm the sensor was recognized and on the right pins before trying the library
* My next goal is to get this working on an arbitrary pin so that I don't have to displace the DHT22 and BME280 and PMS7003 on the other boards if I want to use this. 
  * The .h file shows two ways to initalize, `boolean begin(void);` that returns a boolean and `VEML6075_error_t begin(TwoWire &wirePort);` that returns an enum defined above which has some different error codes. It takes in a TwoWire with a wire port, not exactly sure what that means. However, in the .cpp file it appeards that the simple begin is just a wrapper that passes Wire into the more complex input. `if (begin(Wire) == VEML6075_ERROR_SUCCESS)` so if I can just define Wire on a different port I think I'll be OK
  * https://www.arduino.cc/en/reference/wire has some information on wire
  * `Wire.begin(D1, D2); /* join i2c bus with SDA=D1 and SCL=D2 of NodeMCU */` was retrieved from https://www.electronicwings.com/nodemcu/nodemcu-i2c-with-arduino-ide which gives us the syntax I guess
  * https://www.esp8266.com/viewtopic.php?f=32&t=10899 has some information, it notes Wire is global so I'd have to initialize something else such as TwoWire, aka `TwoWire Wire2 = TwoWire();` and then `Wire2.begin(12,13);`
  * BUT apparently https://electronics.stackexchange.com/questions/25278/how-to-connect-multiple-i2c-interface-devices-into-a-single-pin-a4-sda-and-a5 all the SDAs and SCLs can be wired physically together without using separate pins, and the addressing functionality will keep them working. If I understand correctly, then the BMP/UV sensor could all techincally be on the same SDA and SCL pins and still work fine. 
* I was able to use the D1/D2 pins the way I already had for the BMx280. Instead of routing right to the BMx280 I had it go to a breadboard that allowed me to split the SDA/SCL/VCC/GND between the two sensor boards. They were both picked up completely normally, nice!



### Using

* The webpage output in its current state appears as the following

* ```
  dhtHumidityPercent,dhtTemperatureC,dhtTemperatureF,boschHumidityPercent,boschTemperatureC,boschTemperatureF,boschPressurePa,boschPressureInHg,pmsPm10Standard,pmsPm25Standard,pmsPm100Standard,pmsPm10Environmental,pmsPm25Environmental,pmsPm100Environmental
  58.60,16.70,62.06,55.33,14.00,57.20,101017.17,29.83, 0, 1, 1, 0, 1, 1
  ```

* 