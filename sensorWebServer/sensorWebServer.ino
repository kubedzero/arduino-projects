// libraries needed for OTA web server and WiFi connect
#include <ESP8266WiFi.h> // ESP8266 specific WiFi library
#include <WiFiClient.h> // library supporting WiFi connection
#include <ESP8266WebServer.h> // library for HTTP Server
#include <ESP8266HTTPUpdateServer.h> // library for update code

#include "myCredentials.h" //used to store WiFi and update credentials

// libraries for sensor reading
#include <Wire.h> // for BMP280 or BME280 via I2C
#include <Adafruit_Sensor.h> // Adafruit unified sensor library
#include <Adafruit_BMP280.h> // Adafruit extension library for BMP280


// OTA update constants
const char* update_path = "/firmware"; // URL path to get to firmware update
const char* update_username = WEB_UPDATE_USER; // from creds file
const char* update_password = WEB_UPDATE_PASS; // from creds file
const char* ssid = WIFI_SSID; // from creds file
const char* password = WIFI_PASSWD; // from creds file


#define SERIAL_BAUD 115200 //baud rate for Serial debugging

// Sensor inits and constants
Adafruit_BMP280 bosch; // I2C

ESP8266WebServer httpServer(80);
ESP8266HTTPUpdateServer httpUpdater;

void setupBosch() {


  // BMP280 initialization
  unsigned boschStatus;

  boschStatus = bosch.begin(0x76, 0x58); //BMP280 has I2C address 0x76 and chipID of 0x58
  if (!boschStatus) {
    Serial.println("Could not find a valid Bosch sensor, check wiring, address, sensor ID!");
    //Serial.print("SensorID was: 0x"); Serial.println(bosch.sensorID(),16);
    Serial.print("        ID of 0xFF probably means a bad address, a BMP 180 or BMP 085\n");
    Serial.print("   ID of 0x56-0x58 represents a BMP 280,\n");
    Serial.print("        ID of 0x60 represents a BME 280.\n");
    Serial.print("        ID of 0x61 represents a BME 680.\n");
    while (1);
  }

  /* Default settings from datasheet. */
  bosch.setSampling(Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */
                    Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
                    Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
                    Adafruit_BMP280::FILTER_X16,      /* Filtering. */
                    Adafruit_BMP280::STANDBY_MS_500); /* Standby time. */
}

void setup(void) {
  pinMode(LED_BUILTIN, OUTPUT); // Blue(GeekCreit) or Red(NodeMCU 0.9) LED initialized LOW (LED ON)

  Serial.begin(SERIAL_BAUD);
  Serial.println("\nBooting Sketch...\n");


  // Connect to WiFi network
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.print("\nConnected, IP address is ");
  Serial.println(WiFi.localIP());

  httpUpdater.setup(&httpServer, update_path, update_username, update_password); // OTA server setup

  httpServer.begin();

  digitalWrite(LED_BUILTIN, HIGH); //set Blue(GeekCreit) or Red(NodeMCU 0.9) LED to off

  setupBosch();
}

void loop(void) {
  httpServer.handleClient();
  Serial.print(F("Temperature = "));
  Serial.print(bosch.readTemperature());
  Serial.println(" *C");

  Serial.print(F("Pressure = "));
  Serial.print(bosch.readPressure());
  Serial.println(" Pa");

  Serial.print(F("Approx altitude = "));
  Serial.print(bosch.readAltitude(1013.25)); /* Adjusted to local forecast! */
  Serial.println(" m");

  Serial.println();
  delay(2000);
}
