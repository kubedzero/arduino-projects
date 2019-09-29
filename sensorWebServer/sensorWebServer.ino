// libraries needed for OTA web server and WiFi connect
#include <ESP8266WiFi.h> // ESP8266 specific WiFi library
#include <WiFiClient.h> // library supporting WiFi connection
#include <ESP8266WebServer.h> // library for HTTP Server
#include <ESP8266HTTPUpdateServer.h> // library for update code

#include "myCredentials.h" // used to store WiFi and update credentials

// libraries for sensor reading
#include <Wire.h> // for BMP280 or BME280 via I2C
#include <Adafruit_Sensor.h> // Adafruit unified sensor library
#include <Adafruit_BMP280.h> // Adafruit extension library for BMP280
#include <Adafruit_BME280.h> // Adafruit extension library for BME280
#include <DHT.h> // Adafruit extension library for DHT22

// other libraries
#include <string.h> // string comparison
#include <TaskScheduler.h> // library by arkhipenko for periodic data updates
#include <ArduinoLog.h> // library by thijse for outputting different log levels
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
const char* update_path = "/firmware"; // URL path to get to firmware update
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
const uint8_t pmsRetryCountLimit = 5;

Adafruit_BMP280 bmp280; // I2C BMP280 init
Adafruit_BME280 bme280; // I2C BME280 init
DHT dht(DHTPIN, DHTTYPE); // Initialize DHT sensor.
SoftwareSerial pmsDigitalSerial(PMSRX, PMSTX); // RX, TX to plug TX, RX of PMS into

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

struct pms7003data {
  uint16_t framelen;
  uint16_t pm10_standard, pm25_standard, pm100_standard;
  uint16_t pm10_env, pm25_env, pm100_env;
  uint16_t particles_03um, particles_05um, particles_10um, particles_25um, particles_50um, particles_100um;
  uint16_t unused;
  uint16_t checksum;
};

struct pms7003data pmsData;

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
  // Get DHT Data
  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (it is a very slow sensor)
  dhtHumidityPercent = dht.readHumidity();
  dhtTemperatureC = dht.readTemperature();
  dhtTemperatureF = dht.readTemperature(true);
  Log.verbose("Retrieved new DHT data");

  // Get Bosch Data
  if (boschStatus.equals("BMP280")) {
    boschHumidityPercent = 1.0F / 0.0F; // intentionally set this to NaN
    boschTemperatureC = bmp280.readTemperature();
    boschTemperatureF = boschTemperatureC * (9.0F / 5.0F) + 32.0F;
    boschPressurePa = bmp280.readPressure();
    boschPressureInHg = boschPressurePa / 3386.38866F;
    Log.verbose("Retrieved new BMP280 data");

  } else if (boschStatus.equals("BME280")) {
    boschHumidityPercent = bme280.readHumidity();
    boschTemperatureC = bme280.readTemperature();
    boschTemperatureF = boschTemperatureC * (9.0F / 5.0F) + 32.0F;
    boschPressurePa = bme280.readPressure();
    boschPressureInHg = boschPressurePa / 3386.38866F; // hPA conversion is Pa / 100.0
    Log.verbose("Retrieved new BME280 data");
  } else if (boschStatus.equals("uninitialized")) {
    Log.notice("Could not find a valid BMx280, check wiring, address, sensor ID!");

  }

  // Get PMS Data
  if (pmsStatus.equals("PMS7003") && readPmsData(&pmsDigitalSerial)) {
    pmsPm10Standard = pmsData.pm10_standard;
    pmsPm25Standard = pmsData.pm25_standard;
    pmsPm100Standard = pmsData.pm100_standard;
    pmsPm10Environmental = pmsData.pm10_env;
    pmsPm25Environmental = pmsData.pm25_env;
    pmsPm100Environmental = pmsData.pm100_env;
    Log.verbose("Retrieved new PMS7003 data");
  } 
}

// One-time Arduino setup method
void setup(void) {

  pinMode(LED_BUILTIN, OUTPUT); // Blue(GeekCreit) or Red(NodeMCU 0.9) LED initialized LOW (LED ON)
  // Serial and logging setup
  Serial.begin(SERIAL_BAUD);
  while (!Serial && !Serial.available()) {}
  delay(200); //add some delay before we start printing
  Serial.println(); // get off the junk line
  Log.begin(LOG_LEVEL, &Serial);
  Log.setSuffix(printNewline); // put a newline after each log statement
  Log.notice("Booting Sketch...");

  // Connect to WiFi network
  Log.notice("Connecting to %s", ssid);
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

  setupBosch(); // run the setup method to initialize one Bosch sensor
  dht.begin(); // initialize the DHT sensor
  pmsDigitalSerial.begin(PMS_BAUD); // Start the SoftSerial for the PMS sensor

  // Give the PMS sensor a few attempts to establish a connection
  for (uint8_t i = 1; i <= pmsRetryCountLimit; i++) {
    delay(500);
    if (pmsDigitalSerial && pmsDigitalSerial.available()) {
      pmsStatus = "PMS7003";
      Log.notice("Found PMS7003 on attempt %i!", i);
      break;
    } else {
      Log.notice("PMS7003 not found, %i tries remaining", pmsRetryCountLimit - i);
    }
  }
  if (pmsStatus.equals("uninitialized")) {
    Log.notice("Could not find a valid PMS7003, check wiring, SoftSerial!");

  }

  scheduler.addTask(dataUpdateTask);
  dataUpdateTask.enable();

  httpUpdater.setup(&httpServer, update_path, update_username, update_password); // OTA server setup
  httpServer.onNotFound(handleNotFound);        // When a client requests an unknown URI (i.e. something other than "/"), call function "handleNotFound"
  httpServer.on("/", handleRoot);
  httpServer.begin();
}

// Looping Arduino method
void loop(void) {
  httpServer.handleClient(); // if the webserver is accessed, this handles the request
  scheduler.execute(); // keep the timer running for scheduled data updates
}


// method to connect to and initialize whichever Bosch sensor is connected
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
    Log.notice("BME280 found!\n");
    boschStatus = "BME280";
    bme280.setSampling(Adafruit_BME280::MODE_NORMAL, /* Operating Mode. */
                       Adafruit_BME280::SAMPLING_X16, /* Temp. oversampling */
                       Adafruit_BME280::SAMPLING_X16,  /* Pressure oversampling */
                       Adafruit_BME280::SAMPLING_X16, /* Humidity oversampling */
                       Adafruit_BME280::FILTER_OFF, /* Filtering. */
                       Adafruit_BME280::STANDBY_MS_500); /* Standby time. */
    return;
  }

  // Try BMP280 setup
  boschBeginReturn = bmp280.begin(0x76, 0x58); //BMP280 has I2C address 0x76 and chipID of 0x58
  if (boschBeginReturn) {
    Log.notice("BMP280 found!\n");
    boschStatus = "BMP280";
    /* Default settings from datasheet. */
    bmp280.setSampling(Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */
                       Adafruit_BMP280::SAMPLING_X16,     /* Temp. oversampling */
                       Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
                       Adafruit_BMP280::FILTER_X16,      /* Filtering. */
                       Adafruit_BMP280::STANDBY_MS_500); /* Standby time. */
  }

}

// LED activates when this is hit. Fetches latest data values and serves them with a header
void handleRoot() {
  digitalWrite(LED_BUILTIN, LOW); // set Blue(GeekCreit) or Red(NodeMCU 0.9) LED to on
  Log.notice("Serving root webpage at %l milliseconds", millis()); //print out milliseconds since program launch, resets every 50d
  httpServer.send(200, "text/plain", getGlobalDataHeader() + String("\n") + getGlobalDataString());   // Send HTTP status 200 (Ok) and send some text to the browser/client
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

// PMS data fetcher adapted from https://github.com/adafruit/Adafruit_Learning_System_Guides/blob/master/PMS5003_Air_Quality_Sensor/PMS5003_Arduino/PMS5003_Arduino.ino
boolean readPmsData(Stream * s) {
  if (! s->available()) {
    Log.warning("PMS Stream not available");
    return false;
  }

  // Read a byte at a time until we get to the special '0x42' start-byte
  if (s->peek() != 0x42) {
    s->read();
    return false;
  }

  // Now read all 32 bytes
  if (s->available() < 32) {
    return false;
  }
  uint8_t buffer[32];
  uint16_t sum = 0;
  s->readBytes(buffer, 32);

  // get checksum ready
  for (uint8_t i = 0; i < 30; i++) {
    sum += buffer[i];
  }

  // The data comes in endian'd, this solves it so it works on all platforms
  uint16_t buffer_u16[15];
  for (uint8_t i = 0; i < 15; i++) {
    buffer_u16[i] = buffer[2 + i * 2 + 1];
    buffer_u16[i] += (buffer[2 + i * 2] << 8);
  }

  // put it into a nice struct
  memcpy((void *)&pmsData, (void *)buffer_u16, 30);

  if (sum != pmsData.checksum) {
    Log.warning("PMS checksum failure");
    return false;
  }
  return true;
}
