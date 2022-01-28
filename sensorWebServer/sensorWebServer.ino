// libraries needed for OTA web server and WiFi connect
#include <ESP8266WiFi.h> // 3.0.2 ESP8266 specific WiFi library
#include <WiFiClient.h> // library supporting WiFi connection
#include <ESP8266WebServer.h> // ESP8266 specific library for HTTP Server
#include <ESP8266HTTPUpdateServer.h> // ESP8266 specific library for update code

#include "myCredentials.h" // used to store WiFi and update credentials

// libraries for sensor reading
#include <Adafruit_Sensor.h> // 1.1.4 Adafruit unified sensor library
#include <Adafruit_I2CDevice.h> // 1.10.3 Adafruit BusIO library
#include <Adafruit_BMP280.h> // 2.6.1 Adafruit sensor library for BMP280
#include <Adafruit_BME280.h> // 2.2.2 Adafruit sensor library for BME280
#include <Adafruit_VEML6075.h> // 2.2.0 Adafruit sensor library for VEML6075 UV
#include <Adafruit_SGP30.h> // 2.0.0 Adafruit sensor library for SGP30
#include <DHT.h> // 1.4.3 Adafruit DHT Sensor Library for DHT22
#include <PMS.h> // 1.1.0 Mariusz Kacki (fu-hsi) library for PMS x003 family sensors

// other libraries
#include <string.h> // string comparison
#include <TaskScheduler.h> // 3.6.0 library by arkhipenko for periodic sensor updates
#include <ArduinoLog.h> // 1.1.1 library by thijse for outputting different log levels
#include <SoftwareSerial.h> // library for assigning pins as Serial ports, for PMS7003



// timer setup
void updateSensorData();
Task dataUpdateTask(5000, TASK_FOREVER, &updateSensorData); // run updateSensorData() every 5000ms
Scheduler scheduler;


// log setup
#define SERIAL_BAUD 115200 // baud rate for Serial debugging
// available levels are _SILENT, _FATAL, _ERROR, _WARNING, _NOTICE, _TRACE, _VERBOSE
#define LOG_LEVEL LOG_LEVEL_TRACE


// webserver/OTA update variables and constants
const char* root_path = "/"; // URL path to get to the root page
const char* update_path = "/firmware"; // URL path to get to firmware update
const char* restart_path = "/restart"; // URL path to get to the Restart page
const char* update_username = WEB_UPDATE_USER; // from creds file
const char* update_password = WEB_UPDATE_PASS; // from creds file
const char* ssid = WIFI_SSID; // from creds file
const char* password = WIFI_PASSWD; // from creds file
ESP8266WebServer httpServer(80);
ESP8266HTTPUpdateServer httpUpdater;


// sensor inits, constants, global variables
String boschStatus = "uninitialized";
String pmsStatus = "uninitialized";
String vemlStatus = "uninitialized";
String sgpStatus = "uninitialized";
#define BME280ADDRESS 0x76 // the I2C address of the BME280 used
#define I2CPIN_SDA D2 // the default I2C data pin on an ESP8266
#define I2CPIN_SCL D1 // the default I2C clock pin on an ESP8266
#define DHTPIN D5     // the digital pin for the DHT22. Avoid ESP8266 D4, the ESP_LED pin
#define DHTTYPE DHT22   // options are DHT11, DHT12, DHT22 (AM2302), DHT21 (AM2301)
#define PMSTX D7 // (NOT CONNECTED) what Arduino TX digital pin the PMS sensor RX is connected to
#define PMSRX D6 // what Arduino RX digital pin the PMS sensor TX is connected to
#define PMS_BAUD 9600 // baud rate for SoftwareSerial for PMS7003
#define SENSOR_RETRY_LIMIT 5 // limit the number of connection attempts to any sensor before giving up
#define NO_DATA_INIT_VALUE -16384 // data field initialization value, which should never appear in real readings


Adafruit_BMP280 bmp280; // I2C BMP280 init
Adafruit_BME280 bme280; // I2C BME280 init
Adafruit_SGP30 sgp; // I2C SGP30 init
Adafruit_VEML6075 uv = Adafruit_VEML6075(); // I2C VEML6075 init on the same Wire bus as the BMx280
DHT dht(DHTPIN, DHTTYPE); // initialize DHT sensor.
SoftwareSerial pmsDigitalSerial(PMSRX, PMSTX, false); // [RX, TX] to plug [TX, RX] of PMS into
PMS pms(pmsDigitalSerial); // initialize PMS sensor on specified SoftwareSerial pins.
PMS::DATA pmsData;

// DHT Data
float dhtHumidityPercent = NO_DATA_INIT_VALUE;
float dhtTemperatureC = NO_DATA_INIT_VALUE;

// Bosch Data
float boschHumidityPercent = NO_DATA_INIT_VALUE;
float boschTemperatureC = NO_DATA_INIT_VALUE;
float boschPressurePa = NO_DATA_INIT_VALUE;

// PMS Data
float pmsPm10Standard = NO_DATA_INIT_VALUE;
float pmsPm25Standard = NO_DATA_INIT_VALUE;
float pmsPm100Standard = NO_DATA_INIT_VALUE;
float pmsPm10Environmental = NO_DATA_INIT_VALUE;
float pmsPm25Environmental = NO_DATA_INIT_VALUE;
float pmsPm100Environmental = NO_DATA_INIT_VALUE;

// VEML Data
float vemlUVA = NO_DATA_INIT_VALUE;
float vemlUVB = NO_DATA_INIT_VALUE;
float vemlUVIndex = NO_DATA_INIT_VALUE;

// SGP Data
float sgpTVOC = NO_DATA_INIT_VALUE;
float sgpECO2 = NO_DATA_INIT_VALUE;

// prints the CSV header/schema of the data output
String getGlobalDataHeader() {
  return String("dhtHumidityPercent") + ","
         + String("dhtTemperatureC") + ","
         + String("boschHumidityPercent") + ","
         + String("boschTemperatureC") + ","
         + String("boschPressurePa") + ","
         + String("pmsPm10Standard") + ","
         + String("pmsPm25Standard") + ","
         + String("pmsPm100Standard") + ","
         + String("pmsPm10Environmental") + ","
         + String("pmsPm25Environmental") + ","
         + String("pmsPm100Environmental") + ","
         + String("vemlUVA") + ","
         + String("vemlUVB") + ","
         + String("vemlUVIndex") + ","
         + String("sgpTVOC") + ","
         + String("sgpECO2");
}

// prints the CSV data output of each metric, with defined decimal precision
String getGlobalDataString() {
  return String(dhtHumidityPercent, 2) + ","
         + String(dhtTemperatureC, 2) + ","
         + String(boschHumidityPercent, 2) + ","
         + String(boschTemperatureC, 2) + ","
         + String(boschPressurePa, 2) + ","
         + String(pmsPm10Standard, 0) + ","
         + String(pmsPm25Standard, 0) + ","
         + String(pmsPm100Standard, 0) + ","
         + String(pmsPm10Environmental, 0) + ","
         + String(pmsPm25Environmental, 0) + ","
         + String(pmsPm100Environmental, 0) + ","
         + String(vemlUVA, 2) + ","
         + String(vemlUVB, 2) + ","
         + String(vemlUVIndex, 2) + ","
         + String(sgpTVOC, 0) + ","
         + String(sgpECO2, 0);
}

// logic to handle updating the global sensor data vars from present sensors
void updateSensorData() {
  // get DHT Data. Reading temperature or humidity takes about 250 milliseconds!
  // sensor readings may also be up to 2 seconds 'old' (it is a very slow sensor)
  // If DHT is not connected, the return value will be `nan`
  float tempDhtHumidityPercent = dht.readHumidity();
  float tempDhtTemperatureC = dht.readTemperature();
  if (!isnan(tempDhtHumidityPercent) || !isnan(tempDhtTemperatureC)) {
    dhtHumidityPercent = tempDhtHumidityPercent;
    dhtTemperatureC = tempDhtTemperatureC;
    Log.trace("Retrieved new DHT data");
  }

  // get Bosch Data
  if (boschStatus.equals("BMP280")) {
    boschTemperatureC = bmp280.readTemperature();
    boschPressurePa = bmp280.readPressure();
    Log.trace("Retrieved new BMP280 data");
  } else if (boschStatus.equals("BME280")) {
    boschHumidityPercent = bme280.readHumidity();
    boschTemperatureC = bme280.readTemperature();
    boschPressurePa = bme280.readPressure();
    Log.trace("Retrieved new BME280 data");
  }

  // get PMS Data
  if (pmsStatus.equals("PMS7003") && pms.readUntil(pmsData)) { // readUntil has 1000ms default timeout to get data
    pmsPm10Standard = pmsData.PM_SP_UG_1_0;
    pmsPm25Standard = pmsData.PM_SP_UG_2_5;
    pmsPm100Standard = pmsData.PM_SP_UG_10_0;
    pmsPm10Environmental = pmsData.PM_AE_UG_1_0;
    pmsPm25Environmental = pmsData.PM_AE_UG_2_5;
    pmsPm100Environmental = pmsData.PM_AE_UG_10_0;
    Log.trace("Retrieved new PMS7003 data");
  }

  // get VEML UV data
  if (vemlStatus.equals("VEML6075")) {
    vemlUVA = uv.readUVA();
    vemlUVB = uv.readUVB();
    vemlUVIndex = uv.readUVI();
    Log.trace("Retrieved new VEML6075 data");
  }

  // get SGP TVOC and eCO2 data
  if (sgpStatus.equals("SGP30")) {
    if (sgp.IAQmeasure()) {
      sgpTVOC = sgp.TVOC;
      sgpECO2 = sgp.eCO2;
      Log.trace("Retrieved new SGP30 data");
    } else {
      Log.error("SGP30 Measurement Failed");
    }
  }
}

// one-time Arduino setup method
void setup(void) {
  pinMode(LED_BUILTIN, OUTPUT); // Blue(GeekCreit) or Red(NodeMCU 0.9) LED initialized LOW (LED ON)
  // serial, logging, WiFi setup
  Serial.begin(SERIAL_BAUD);
  while (!Serial && !Serial.available()) {}
  delay(1000); // add some delay before we start printing
  Serial.println(); // get off the junk serial console line
  Log.begin(LOG_LEVEL, &Serial);
  Log.setSuffix(printNewline); // put a newline after each log statement
  Log.notice("Booting Sketch...");
  WiFiConnect(); // do the WiFi initialization

  dht.begin(); // initialize the DHT sensor. No extra handling, as it returns void
  setupBosch(); // run the setup method to initialize one Bosch sensor
  setupPMS(); // run the setup method to initialize the PMS sensor
  setupVEML(); // run the setup method to initialize the VEML sensor
  setupSGP(); // run the setup method to initialize the SGP sensor

  scheduler.addTask(dataUpdateTask); // initialize the scheduled data gathering task
  dataUpdateTask.enable(); // enable the data gathering task

  httpUpdater.setup(&httpServer, update_path, update_username, update_password); // OTA server setup
  httpServer.onNotFound(handleNotFound); // when a client requests an unknown URI (i.e. something other than "/"), call function "handleNotFound"
  httpServer.on(root_path, handleRoot); // take care of the page we populate with our information
  httpServer.on(restart_path, handleRestart); // calling this page will trigger a restart/reboot of the ESP
  httpServer.begin();
}

// looping Arduino method
void loop(void) {
  WiFiConnect(); // make sure the WiFi maintains connection. Library could do this but mine has LED indicatiors
  httpServer.handleClient(); // if the webserver is accessed, this handles the request
  scheduler.execute(); // keep the timer running for scheduled data updates
}

// connect or reconnect to WiFi network
void WiFiConnect() {
  // Attempt a reconnect any time we are not in a connected state. No timeout
  if (WiFi.status() != WL_CONNECTED)
  {
    digitalWrite(LED_BUILTIN, LOW); // set Blue(GeekCreit) or Red(NodeMCU 0.9) LED to on while connecting
    Log.notice("WiFi is not connected. Connecting to %s", ssid);
    WiFi.disconnect();
    WiFi.mode(WIFI_STA); // set Station mode, rater than access point mode. WIFI_AP, WIFI_STA, WIFI_AP_STA or WIFI_OFF
    WiFi.begin(ssid, password); // connect to wifi with the provided credentials
    while (WiFi.status() != WL_CONNECTED) {
      delay(1000);
      Log.notice("waiting...");
    }
    String ipAddrString = WiFi.localIP().toString(); // convert the IPAddress opject to a String
    char * ipAddrChar = new char [ipAddrString.length() + 1]; // allocate space for a char array
    strcpy (ipAddrChar, ipAddrString.c_str()); // populate the char array
    Log.notice("Connected, IP address is %s", ipAddrChar); // pass the char array to the logger
    digitalWrite(LED_BUILTIN, HIGH); // set Blue(GeekCreit) or Red(NodeMCU 0.9) LED to off
  } else {
    Log.verbose("WiFi is connected, nothing to do");
  }
}

// method to connect and initialize whichever Bosch sensor is connected
void setupBosch() {
  unsigned boschBeginReturn;
  // try BME280 setup
  boschBeginReturn = bme280.begin(BME280ADDRESS, &Wire);
  Log.trace("BME reports the wire sensorID as: 0x%l", bme280.sensorID());
  Log.trace("ID of 0xFF/255 could be a bad address, a BMP 180 or BMP 085");
  Log.trace("ID of 0x56-0x58 represents a BMP 280");
  Log.trace("ID of 0x60/76 represents a BME 280");
  Log.trace("ID of 0x61 represents a BME 680");
  if (!boschBeginReturn) {
    Log.notice("No BME280 found, moving to check next Bosch sensor");
  } else {
    Log.notice("BME280 found!");
    boschStatus = "BME280";
    bme280.setSampling(Adafruit_BME280::MODE_NORMAL, // Operating Mode
                       Adafruit_BME280::SAMPLING_X16, // Temp. oversampling
                       Adafruit_BME280::SAMPLING_X16, // Pressure oversampling
                       Adafruit_BME280::SAMPLING_X16, // Humidity oversampling
                       Adafruit_BME280::FILTER_OFF, // Filtering
                       Adafruit_BME280::STANDBY_MS_500); // Standby time
    return; // exit early as we've found a sensor
  }

  // try BMP280 setup
  boschBeginReturn = bmp280.begin(0x76, 0x58); // BMP280 has I2C address 0x76 and chipID of 0x58
  if (boschBeginReturn) {
    Log.notice("BMP280 found!");
    boschStatus = "BMP280";
    bmp280.setSampling(Adafruit_BMP280::MODE_NORMAL, // Operating Mode
                       Adafruit_BMP280::SAMPLING_X16, // Temp. oversampling
                       Adafruit_BMP280::SAMPLING_X16, // Pressure oversampling
                       Adafruit_BMP280::FILTER_X16, // Filtering
                       Adafruit_BMP280::STANDBY_MS_500); // Standby time
  }
}

// method to connect and initialize a PMS7003 sensor sensor
void setupPMS() {
  pmsDigitalSerial.begin(PMS_BAUD); // start the SoftSerial for the PMS sensor

  // give the PMS sensor a few attempts to establish a connection
  for (uint8_t i = 1; i <= SENSOR_RETRY_LIMIT; i++) {
    if (pmsDigitalSerial && pmsDigitalSerial.available()) {
      pmsStatus = "PMS7003";
      Log.notice("Found PMS7003 on attempt %i!", i);
      break; // exit the loop early as we've found the sensor
    } else {
      Log.notice("PMS7003 not found, %i tries remaining", SENSOR_RETRY_LIMIT - i);
    }
    delay(500);
  }
  if (pmsStatus.equals("uninitialized")) {
    Log.notice("Could not find a valid PMS7003! Check wiring, SoftSerial!");
  }
}

// method to connect and initialize a VEML6075 sensor
void setupVEML() {
  // give the VEML sensor a few attempts to establish a connection
  for (uint8_t i = 1; i <= SENSOR_RETRY_LIMIT; i++) {
    if (uv.begin()) {
      vemlStatus = "VEML6075";
      Log.notice("Found VEML6075 on attempt %i!", i);
      break; // exit the loop early as we've found the sensor
    } else {
      Log.notice("VEML6075 not found, %i tries remaining", SENSOR_RETRY_LIMIT - i);
    }
    delay(500);
  }
  if (vemlStatus.equals("uninitialized")) {
    Log.notice("Could not find a valid VEML6075, check wiring, I2C bus!");
  }
}

// method to connect and initialize an SGP30 sensor
void setupSGP() {
  // give the SGP sensor a few attempts to establish a connection
  for (uint8_t i = 1; i <= SENSOR_RETRY_LIMIT; i++) {
    if (sgp.begin(&Wire, true)) {
      sgpStatus = "SGP30";
      Log.notice("Found SGP30 on attempt %i!", i);
      // print serial using hex to ascii wildcard, example had Serial.print(sgp.serialnumber[0], HEX);
      Log.notice("SGP30 serial is #%x%x%x", sgp.serialnumber[0], sgp.serialnumber[1], sgp.serialnumber[2]);
      break; // exit the loop early as we've found the sensor
    } else {
      Log.notice("SGP30 not found, %i tries remaining", SENSOR_RETRY_LIMIT - i);
    }
    delay(500);
  }
  if (sgpStatus.equals("uninitialized")) {
    Log.notice("Could not find a valid SGP30, check wiring, I2C bus!");
  }
}

// LED activates when this is hit. Fetches latest data values and serves them as two lines of CSV: schema line and data line
void handleRoot() {
  digitalWrite(LED_BUILTIN, LOW); // set Blue(GeekCreit) or Red(NodeMCU 0.9) LED to on
  Log.notice("Serving root webpage at %l milliseconds", millis()); // print out milliseconds since program launch, resets every 50d
  httpServer.send(200, "text/plain", getGlobalDataHeader() + String("\n") + getGlobalDataString());   // send HTTP status 200 (Ok) and send some text to the browser/client
  digitalWrite(LED_BUILTIN, HIGH); // set Blue(GeekCreit) or Red(NodeMCU 0.9) LED to off

}

// LED activates when this is hit. Sends a page with a button that can be used to reboot
void handleRestart() {
  digitalWrite(LED_BUILTIN, LOW); // set Blue(GeekCreit) or Red(NodeMCU 0.9) LED to on
  Log.notice("Serving restart webpage at %l milliseconds", millis()); // print out milliseconds since program launch, resets every 50d
  httpServer.send(200, "text/plain", "A restart/reboot of the ESP is being triggered");   // send HTTP status 200 (Ok) and send some text to the browser/client
  delay(250); // give the HttpServer some time to send the response before restarting
  ESP.restart(); // reboot the ESP
  digitalWrite(LED_BUILTIN, HIGH); // set Blue(GeekCreit) or Red(NodeMCU 0.9) LED to off

}

// LED activates when this is hit. Serves a 404 to the caller
void handleNotFound() {
  digitalWrite(LED_BUILTIN, LOW); // set Blue(GeekCreit) or Red(NodeMCU 0.9) LED to on
  Log.notice("Serving 404 not found webpage at %l milliseconds", millis()); // print out milliseconds since program launch, resets every 50d
  httpServer.send(404, "text/plain", "404: Not found"); // send HTTP status 404 (Not Found) when there's no handler for the URI in the request
  digitalWrite(LED_BUILTIN, HIGH); // set Blue(GeekCreit) or Red(NodeMCU 0.9) LED to off
}

// helper to add a newline at the end of every log statement. Provided by log-advanced example.
void printNewline(Print * _logOutput, int logLevel) {
  _logOutput->print('\n');
}
