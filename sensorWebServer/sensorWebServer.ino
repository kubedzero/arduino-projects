// libraries needed for OTA web server and WiFi connect
#include <ESP8266WiFi.h> // ESP8266 specific WiFi library
#include <WiFiClient.h> // library supporting WiFi connection
#include <ESP8266WebServer.h> // library for HTTP Server
#include <ESP8266HTTPUpdateServer.h> // library for update code

#include "myCredentials.h" // used to store WiFi and update credentials

// libraries for sensor reading
#include <Wire.h> // for BMP280 or BME280 via I2C
#include <Adafruit_Sensor.h> // 1.0.3 Adafruit unified sensor library
#include <Adafruit_BMP280.h> // 1.0.5 Adafruit extension library for BMP280
#include <Adafruit_BME280.h> // 1.0.10 Adafruit extension library for BME280
#include <DHT.h> // 1.3.7 Adafruit extension library for DHT22
#include <PMS.h> // 1.1.0 Mariusz Kacki (fu-hsi) library for PMS x003 family sensors

// other libraries
#include <string.h> // string comparison
#include <TaskScheduler.h> // 3.0.2 library by arkhipenko for periodic data updates
#include <ArduinoLog.h> // 1.0.3 library by thijse for outputting different log levels
#include <SoftwareSerial.h> // library for assigning pins as Serial ports, for PMS7003



// Timer setup
void updateSensorData();
Task dataUpdateTask(5000, TASK_FOREVER, &updateSensorData); // run updateSensorData() every 5000ms
Scheduler scheduler;


// Log setup
#define SERIAL_BAUD 115200 // baud rate for Serial debugging
// available levels are _SILENT, _FATAL, _ERROR, _WARNING, _NOTICE, _TRACE, _VERBOSE
#define LOG_LEVEL LOG_LEVEL_NOTICE


// Webserver/OTA update variables and constants
const char* root_path = "/"; // URL path to get to the root page
const char* update_path = "/firmware"; // URL path to get to firmware update
const char* restart_path = "/restart"; // URL path to get to the Restart page
const char* update_username = WEB_UPDATE_USER; // from creds file
const char* update_password = WEB_UPDATE_PASS; // from creds file
const char* ssid = WIFI_SSID; // from creds file
const char* password = WIFI_PASSWD; // from creds file
ESP8266WebServer httpServer(80);
ESP8266HTTPUpdateServer httpUpdater;


// Sensor inits, constants, global variables
String boschStatus = "uninitialized";
String pmsStatus = "uninitialized";
#define DHTPIN D4     // what digital pin the DHT sensor is connected to
#define DHTTYPE DHT22   // Options are DHT11, DHT12, DHT22 (AM2302), DHT21 (AM2301)
#define PMSTX D7 // (NOT CONNECTED) what Arduino TX digital pin the PMS sensor RX is connected to
#define PMSRX D6 // what Arduino RX digital pin the PMS sensor TX is connected to
#define PMS_BAUD 9600 //baud rate for SoftwareSerial for PMS7003
#define PMS_RETRY_LIMIT 5 // Limit the number of connection attempts to the PMS sensor before giving up


Adafruit_BMP280 bmp280; // I2C BMP280 init
Adafruit_BME280 bme280; // I2C BME280 init
DHT dht(DHTPIN, DHTTYPE); // Initialize DHT sensor.
SoftwareSerial pmsDigitalSerial(PMSRX, PMSTX); // RX, TX to plug TX, RX of PMS into
PMS pms(pmsDigitalSerial);
PMS::DATA pmsData;

// DHT Data
float dhtHumidityPercent = 0;
float dhtTemperatureC = 0;
float dhtTemperatureF = 0;

// Bosch Data
float boschHumidityPercent = 0;
float boschTemperatureC = 0;
float boschTemperatureF = 0;
float boschPressurePa = 0;
float boschPressureInHg = 0;

// PMS Data
float pmsPm10Standard = 0;
float pmsPm25Standard = 0;
float pmsPm100Standard = 0;
float pmsPm10Environmental = 0;
float pmsPm25Environmental = 0;
float pmsPm100Environmental = 0;

// prints the CSV header/schema of the data output
String getGlobalDataHeader() {
  return String("dhtHumidityPercent,")
         + String("dhtTemperatureC,")
         + String("dhtTemperatureF,")
         + String("boschHumidityPercent,")
         + String("boschTemperatureC,")
         + String("boschTemperatureF,")
         + String("boschPressurePa,")
         + String("boschPressureInHg,")
         + String("pmsPm10Standard,")
         + String("pmsPm25Standard,")
         + String("pmsPm100Standard,")
         + String("pmsPm10Environmental,")
         + String("pmsPm25Environmental,")
         + String("pmsPm100Environmental");
}

// prints the CSV data output of each metric
String getGlobalDataString() {
  return String(dhtHumidityPercent, 2) + ","
         + String(dhtTemperatureC, 2) + ","
         + String(dhtTemperatureF, 2) + ","
         + String(boschHumidityPercent, 2) + ","
         + String(boschTemperatureC, 2) + ","
         + String(boschTemperatureF, 2) + ","
         + String(boschPressurePa, 2) + ","
         + String(boschPressureInHg, 2) + ","
         + String(pmsPm10Standard, 0) + ","
         + String(pmsPm25Standard, 0) + ","
         + String(pmsPm100Standard, 0) + ","
         + String(pmsPm10Environmental, 0) + ","
         + String(pmsPm25Environmental, 0) + ","
         + String(pmsPm100Environmental, 0);
}

// Logic to handle updating the global sensor data vars from present sensors
void updateSensorData() {
  // Get DHT Data. Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (it is a very slow sensor)
  dhtHumidityPercent = dht.readHumidity();
  dhtTemperatureC = dht.readTemperature();
  dhtTemperatureF = dht.readTemperature(true);
  if (!isnan(dhtHumidityPercent) || !isnan(dhtTemperatureC) || !isnan(dhtTemperatureF)) {
    Log.trace("Retrieved new DHT data");
  }

  // Get Bosch Data
  if (boschStatus.equals("BMP280")) {
    boschHumidityPercent = 1.0F / 0.0F; // intentionally set this to inf to signify no reading
    boschTemperatureC = bmp280.readTemperature();
    boschTemperatureF = boschTemperatureC * (9.0F / 5.0F) + 32.0F; // manual *C to *F conversion
    boschPressurePa = bmp280.readPressure();
    boschPressureInHg = boschPressurePa / 3386.38866F; // manual Pascals to inches of Mercury conversion
    Log.trace("Retrieved new BMP280 data");

  } else if (boschStatus.equals("BME280")) {
    boschHumidityPercent = bme280.readHumidity();
    boschTemperatureC = bme280.readTemperature();
    boschTemperatureF = boschTemperatureC * (9.0F / 5.0F) + 32.0F;
    boschPressurePa = bme280.readPressure();
    boschPressureInHg = boschPressurePa / 3386.38866F; // manual Pascals to inches of Mercury conversion
    Log.trace("Retrieved new BME280 data");
  } else if (boschStatus.equals("uninitialized")) {
    Log.notice("Could not find a valid BMx280, check wiring, address, sensor ID!");

  }

  // Get PMS Data
  if (pmsStatus.equals("PMS7003") && pms.readUntil(pmsData)) { // readUntil has 1000ms default timeout to get data
    pmsPm10Standard = pmsData.PM_SP_UG_1_0;
    pmsPm25Standard = pmsData.PM_SP_UG_2_5;
    pmsPm100Standard = pmsData.PM_SP_UG_10_0;
    pmsPm10Environmental = pmsData.PM_AE_UG_1_0;
    pmsPm25Environmental = pmsData.PM_AE_UG_2_5;
    pmsPm100Environmental = pmsData.PM_AE_UG_10_0;
    Log.trace("Retrieved new PMS7003 data");
  }
}

// One-time Arduino setup method
void setup(void) {
  pinMode(LED_BUILTIN, OUTPUT); // Blue(GeekCreit) or Red(NodeMCU 0.9) LED initialized LOW (LED ON)
  // Serial, logging, WiFi setup
  Serial.begin(SERIAL_BAUD);
  while (!Serial && !Serial.available()) {}
  delay(200); //add some delay before we start printing
  Serial.println(); // get off the junk line
  Log.begin(LOG_LEVEL, &Serial);
  Log.setSuffix(printNewline); // put a newline after each log statement
  Log.notice("Booting Sketch...");
  WiFiConnect();

  setupBosch(); // run the setup method to initialize one Bosch sensor
  dht.begin(); // initialize the DHT sensor
  pmsDigitalSerial.begin(PMS_BAUD); // Start the SoftSerial for the PMS sensor

  // Give the PMS sensor a few attempts to establish a connection
  for (uint8_t i = 1; i <= PMS_RETRY_LIMIT; i++) {
    delay(500);
    if (pmsDigitalSerial && pmsDigitalSerial.available()) {
      pmsStatus = "PMS7003";
      Log.notice("Found PMS7003 on attempt %i!", i);
      break; // exit the loop early as we've found the sensor
    } else {
      Log.notice("PMS7003 not found, %i tries remaining", PMS_RETRY_LIMIT - i);
    }
  }
  if (pmsStatus.equals("uninitialized")) {
    Log.notice("Could not find a valid PMS7003, check wiring, SoftSerial!");
  }

  scheduler.addTask(dataUpdateTask); // initialize the scheduled data gathering task
  dataUpdateTask.enable(); // enable the data gathering task

  httpUpdater.setup(&httpServer, update_path, update_username, update_password); // OTA server setup
  httpServer.onNotFound(handleNotFound); // When a client requests an unknown URI (i.e. something other than "/"), call function "handleNotFound"
  httpServer.on(root_path, handleRoot); // take care of the page we populate with our information
  httpServer.on(restart_path, handleRestart); // Calling this page will trigger a restart/reboot of the ESP
  httpServer.begin();
}

// Looping Arduino method
void loop(void) {
  WiFiConnect(); // Make sure the WiFi maintains connection. Library does this for us but this has LED notification
  httpServer.handleClient(); // if the webserver is accessed, this handles the request
  scheduler.execute(); // keep the timer running for scheduled data updates
}

// Connect or reconnect to WiFi network
void WiFiConnect() {
  if (WiFi.status() != WL_CONNECTED)
  {
    digitalWrite(LED_BUILTIN, LOW); // set Blue(GeekCreit) or Red(NodeMCU 0.9) LED to on
    Log.notice("WiFi is not connected. Connecting to %s", ssid);
    WiFi.disconnect();
    WiFi.mode(WIFI_AP_STA);
    WiFi.begin(ssid, password);
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
  // Try BME280 setup
  boschBeginReturn = bme280.begin();
  Log.trace("Wire SensorID was: 0x%l", bme280.sensorID());
  Log.trace("ID of 0xFF could be a bad address, a BMP 180 or BMP 085");
  Log.trace("ID of 0x56-0x58 represents a BMP 280");
  Log.trace("ID of 0x60 represents a BME 280");
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

  // Try BMP280 setup
  boschBeginReturn = bmp280.begin(0x76, 0x58); //BMP280 has I2C address 0x76 and chipID of 0x58
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

// LED activates when this is hit. Fetches latest data values and serves them with a header
void handleRoot() {
  digitalWrite(LED_BUILTIN, LOW); // set Blue(GeekCreit) or Red(NodeMCU 0.9) LED to on
  Log.notice("Serving root webpage at %l milliseconds", millis()); //print out milliseconds since program launch, resets every 50d
  httpServer.send(200, "text/plain", getGlobalDataHeader() + String("\n") + getGlobalDataString());   // Send HTTP status 200 (Ok) and send some text to the browser/client
  digitalWrite(LED_BUILTIN, HIGH); // set Blue(GeekCreit) or Red(NodeMCU 0.9) LED to off

}
// LED activates when this is hit. Sends a page with a button that can be used to reboot
void handleRestart() {
  digitalWrite(LED_BUILTIN, LOW); // set Blue(GeekCreit) or Red(NodeMCU 0.9) LED to on
  Log.notice("Serving restart webpage at %l milliseconds", millis()); //print out milliseconds since program launch, resets every 50d
  httpServer.send(200, "text/plain", "A restart/reboot of the ESP is being triggered");   // Send HTTP status 200 (Ok) and send some text to the browser/client
  delay(250); // Give the HttpServer some time to send the response before restarting
  ESP.restart(); // Reboot the ESP
  digitalWrite(LED_BUILTIN, HIGH); // set Blue(GeekCreit) or Red(NodeMCU 0.9) LED to off

}
// LED activates when this is hit. Serves a 404 to the caller
void handleNotFound() {
  digitalWrite(LED_BUILTIN, LOW); // set Blue(GeekCreit) or Red(NodeMCU 0.9) LED to on
  Log.notice("Serving 404 not found webpage at %l milliseconds", millis()); //print out milliseconds since program launch, resets every 50d
  httpServer.send(404, "text/plain", "404: Not found"); // Send HTTP status 404 (Not Found) when there's no handler for the URI in the request
  digitalWrite(LED_BUILTIN, HIGH); // set Blue(GeekCreit) or Red(NodeMCU 0.9) LED to off
}

// helper to add a newline at the end of every log statement
void printNewline(Print * _logOutput) {
  _logOutput->print('\n');
}
