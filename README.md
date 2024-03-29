# ESP8266 Monitoring Code

## OTA Update

* using http://arduino.esp8266.com/Arduino/versions/2.0.0/doc/ota_updates/ota_updates.html#web-browser as the base of the sketch, will allow over the air updates without having to plug into the computer. 
  * Base sketch in the ESP8266HTTPUpdateServer folder of Examples, using the SecureWebUpdater which requires a username and password to allow update
  * Sketch mentions a curl command that can be used to update `curl -u username:password -F "image=@firmware.bin" esp8266-webupdate.local/firmware` 
  * http://arduino.esp8266.com/Arduino/versions/2.0.0/doc/ota_updates/ota_updates.html#implementation-overview provides info on how to integrate it with your normal sketch
  * Compile log shows towards the end `Creating BIN file "/var/folders/gt/b5zkfzhj0z74_q1z2ngthjt00000gn/T/arduino_build_133328/otaUpdate.ino.bin" using...` so I can use CMD + G to go to that location in the web browser http://ipaddress/firmware or I can run the curl command `curl -u myUsername:myPassword -F "image=@/var/folders/gt/b5zkfzhj0z74_q1z2ngthjt00000gn/T/arduino_build_133328/otaUpdate.ino.bin" ip.address/firmware` 
  * The output location of the BIN doesn't seem to change between recompiles
* https://arduino.stackexchange.com/questions/40411/hiding-wlan-password-when-pushing-to-github suggests a way to store Wifi passwords and stuff outside the normal file. 
* Git ignore all files with same name https://stackoverflow.com/questions/8981472/ignore-all-files-with-the-same-name-in-git
* WiFi reconnect method idea https://www.esp8266.com/viewtopic.php?f=32&t=8286
* Currently, the ESPs are all also broadcasting a WiFi network. https://arduino-esp8266.readthedocs.io/en/latest/esp8266wifi/soft-access-point-class.html#softapdisconnect seems to describe this as a SoftAP because it's using the same hardware as the WiFi connection. Apparently it can be turned off with `WiFi.softAPdisconnect(true)`. Alternatively, https://arduino-esp8266.readthedocs.io/en/latest/esp8266wifi/readme.html notes we can just initialize Wifi with a different mode, maybe STA instead of `WiFi.mode(WIFI_AP_STA);` https://arduino-esp8266.readthedocs.io/en/latest/esp8266wifi/generic-class.html confirms this, notes that `WiFi.mode(m)`: set mode to `WIFI_AP`, `WIFI_STA`, `WIFI_AP_STA` or `WIFI_OFF`



## BME280 Integration

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

## BMP280 Integration

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

## PMS7003 Integration

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
    * 2023 update: The sensor seems to work better when supplied with 5V, so I'm using the USB/5V supply and no level conversion on an Adafruit Huzzah ESP32 Feather and everything seems to work fine.
  * 2023 update: There would be times when it seemed as if no new readings were succeeding, so I changed the call to time out after 2000ms and that seems to have increased update success rate dramatically. Also, I set it so the sensors report the -16384 error value instead of the same value as before if they encounter an error, so Influx isn't filled with questionably reliable repeat readings
* Refs
  * https://kamami.com/gas-sensors/564008-plantower-pms7003-laser-dust-sensor.html
  * https://www.espruino.com/PMS7003
  * https://aqicn.org/sensor/pms5003-7003/
  * https://learn.adafruit.com/pm25-air-quality-sensor/arduino-code
  * https://github.com/fu-hsi/pms



## VEML6075 UV Sensor Integration

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



## SGP30 TVOC/eCO2 Air Sensor

* using the I2C Scanner sketch with no modifications (didn't need to change the Wire pin or anything) I see 3 I2C outputs after daisy chaining VCC/GND/SCL/SDA from the UV + BMP into the SGP. I see I2C addresses 0x10, 0x58, 0x76 and I believe 58 and 76 were there before. Good to know. 
* Going to give the Adafruit SGP30 library a shot since I'm already using Adafruit for BMP280 and BME280. Cool, I get readings with no issues. This is telling me the TVOC is 0ppb (90 when I breathed on it), raw H2 of 12047, raw ethanol of 16560, and eCO2 of around 470ppm. I found a reading website in Hawaii reporting 414ppm so it seems about right.
* I tried the spark fun library and it reported about the same, 400-410 CO2 ppm and TVOC 0-10ppb, breathing raised  it to 430ppm and 49ppb.
* WHOA dripping some isopropanol nearby made the values go nuts, TVOC of 60000ppb and CO2 of 57000ppm. As expected, pure alcohol vapor isn't going to play nice with the readings
* Sparkfun allows for mySensor.initAirQuality and then my sensor.measureAirQuality, pretty easy. 
* Adafruit allows for sgp.begin and then sgp.IAQmeasure()
* I had an issue with gibberish printing out to the logs, even with the original code. Not sure what happened but I updated some libraries and restarted Arduino and then it worked fine
* 2021Q3 Update: I got a new SGP30 from Adafruit that uses the Stemma QT JST sockets: https://learn.adafruit.com/introducing-adafruit-stemma-qt/technical-specs 
  * Black is ground
  * Red is VCC
  * Blue is SDA
  * Yellow is SCL
* 2022Q1 Update: I wanted the ESP-12 built-in LED to stop blinking every 5 seconds. I didn't have that blinking functionality programmed in, so I assumed it was coming from something else. Sure enough, Pin D4/GPIO2 is where I have the DHT22 connected, and according to https://lowvoltage.github.io/2017/07/09/Onboard-LEDs-NodeMCU-Got-Two that's the same pin used for the LED. I think what was happening was that the DHT was being queried for data, and that activity on pin D4 also triggered the LED. By changing the sketch and wiring to use pin D5 instead, I was able to avoid triggering the LED. 
  * On a completely different note, I had an issue with temperature readings shooting up to 150-180 degrees celsius sometimes, and the pressure going nuts as well. Reading through https://forum.arduino.cc/t/bmp280-sensor-wrong-data-readings/513455/15 showed me that the exposed pins on the BMP280 breakout board could switch between I2C and SPI mode. I experimented with a screwdriver touching the pins and sure enough I could cause the weird behavior to occur on demand. Covering the exposed pins seems to have helped drive down the error rate. 



## SCD40 True CO2 Sensor

- I bought a true CO2 sensor, so instead of the eCO2 values output by the SGP30, this is truly measuring the CO2 amount in the air. It was about $15 from AliExpress, and the equivalent sensor on Adafruit was $45
- https://learn.adafruit.com/adafruit-scd-40-and-scd-41/arduino 
- the sample sketch outputs exactly the sort of data I'd want `Co2:2030	Temperature:23.54	Humidity:66.57` but also surprised me in that it reads temperature and humidity. Very cool!
- There also exists an SCD41 that is slightly more accurate (1% accuracy instead of 5% accuracy, for example) but I don't really need that. 
- Adafruit doesn't have their own SCD4x library so I used the manufacturer library that Adafruit references https://github.com/Sensirion/arduino-i2c-scd4x which also requires the Sensirion Core library to be installed. 
- I found that long/bad jumper cables can cause the SCD40 to either not initialize, or return false for the isDataReady function, so well-established wiring is crucial

## Using the Arduino Sketch Webpage

* The webpage output in its current state appears as the following

* ```
  dhtHumidityPercent,dhtTemperatureC,boschHumidityPercent,boschTemperatureC,boschPressurePa,pmsPm10Standard,pmsPm25Standard,pmsPm100Standard,pmsPm10Environmental,pmsPm25Environmental,pmsPm100Environmental,vemlUVA,vemlUVB,vemlUVIndex,sgpTVOC,sgpECO2,scdCO2,scdTemperatureC,scdHumidityPercent
  -16384.00,-16384.00,44.60,27.21,102026.45,17,22,25,17,22,25,-16384.00,-16384.00,-16384.00,25,431,-16384,-16384.00,-16384.00
  ```



## Default I2C Pins

- I realized that I'm initializing Wire and never defining the pins, which means each board will just choose whatever pins it's configured as defaults for I2C SCL and SDA. 
  - For the ESP32 Adafruit Huzzah Feather, SDA is pin 23 and marked SDA on the board, and SCL is pin 22 https://www.adafruit.com/product/3405
  - For the ESP32 from AliExpress (I chose DOIT ESP32 DEVKIT V1 in the Board Manager), there are no SDA/SCL markings on the board, but documentation and experimentation shows pin 22 is SCL and pin 21 is SDA (on either side of the RX/TX SPI pins) https://www.aliexpress.us/item/3256804701258817.html
  - For the various ESP8266 boards (I choose NodeMCU 1.0 ESP 12E in the Board Manager) SCL is D1 and SDA is D2, right next to each other
- Note that this means a firmware build for the Feather won't work for the DOIT ESP32 because the pinout will be different. Compile and upload separately. This is easy to catch though because I think they use different USB to UART chips (Feather uses the SIL CP2104 and the DOIT uses the CP2102)



## USB Drivers

- Most of the ESP8266s I have, and cheap ESP32s use the cheap WCH brand, model CH340 or CH340G
  - https://learn.sparkfun.com/tutorials/how-to-install-ch340-drivers/all
  - https://www.wch-ic.com/downloads/CH341SER_ZIP.html
  - WCH also makes a CH9102 or CH9102x which offers 4Mbps upload speed 
  - https://www.electronics-lab.com/meet-ch9102f-a-low-cost-alternative-to-cp2104-usb-to-uart-bridge/cp210x-vs-ch340k-vsch9102/
- Nicer boards use a more established Silicon Labs CP2102 or CP2104 (different form factor)
  - No driver should be needed for these, at least on macOS



## `LED_BUILTIN` Behavior

- Pin 13 on the Adafruit Feather is linked to the LED on the board next to the micro USB, at least according to the silkscreen. In operation on my original sketch, it is on all the time.
  - The Feather also has a constantly flashing CHG light indicating it's getting voltage from the USB port
  - The controllable LED is red
  - Looking in the `pins_arduino.h` with the Feather board loaded, I see LED_BUILTIN = 13 which confirms the suspicion. 
- D2 is marked on the DOIT ESP32 which seems to be GPIO2. 
  - Looking in the `pins_arduino.h` with the DOIT board loaded, I see LED_BUILTIN = 2 which confirms the suspicion. 
  - The controllable LED is blue
  - The DOIT also has a red LED that's on all the time to indicate there is power. I sharpie'd over it to dim it a bit
- I found that with ESP32, setting the LED pin to HIGH turns on the LED, which is the opposite of ESP8266.
- Initializing the LED pin sets it to LOW by default, so with the ESP32 code I need to add an extra line to set it to HIGH to turn it on while booting.

## Compiling

- I had an issue where I was seeing an error when compiling sketches for my Adafruit Huzzah ESP32 Feather: `exec: "python": executable file not found in $PATH; Error compiling for board Adafruit ESP32 Feather.` is what Arduino 1.8.19 would print out. I found a suggestion that said to open Arduino from the CLI, meaning `open /Applications/Arduino.app` and sure enough after that it worked fine!
  - I've never had this issue with Arduino IDE 2.x