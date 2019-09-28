# ESP8266 Monitoring Code

### OTA Update

* using http://arduino.esp8266.com/Arduino/versions/2.0.0/doc/ota_updates/ota_updates.html#web-browser as the base of the sketch, will allow over the air updates without having to plug into the computer. 
  * Base sketch in the ESP8266HTTPUpdateServer folder of Examples, using the SecureWebUpdater which requires a username and password to allow update
  * Sketch mentions a curl command that can be used to update `curl -u username:password -F "image=@firmware.bin" esp8266-webupdate.local/firmware` 
  * http://arduino.esp8266.com/Arduino/versions/2.0.0/doc/ota_updates/ota_updates.html#implementation-overview provides info on how to integrate it with your normal sketch
  * Compile log shows towards the end `Creating BIN file "/var/folders/gt/b5zkfzhj0z74_q1z2ngthjt00000gn/T/arduino_build_133328/otaUpdate.ino.bin" using...` so I can use CMD + G to go to that location in the web browser http://ipaddress/firmware or I can run the curl command `curl -u myUsername:myPassword -F "image=@/var/folders/gt/b5zkfzhj0z74_q1z2ngthjt00000gn/T/arduino_build_133328/otaUpdate.ino.bin" ip.address/firmware` 
  * The output location of the BIN doesn't seem to change between recompiles
* https://arduino.stackexchange.com/questions/40411/hiding-wlan-password-when-pushing-to-github suggests a way to store Wifi passwords and stuff outside the normal file. 
* 