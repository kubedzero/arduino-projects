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
#include <Adafruit_BME280.h> // Adafruit extension library for BME280
#include <DHT.h> //Adafruit extension library DHT22

// other libraries
#include <string.h> //string comparison


// OTA update constants
const char* update_path = "/firmware"; // URL path to get to firmware update
const char* update_username = WEB_UPDATE_USER; // from creds file
const char* update_password = WEB_UPDATE_PASS; // from creds file
const char* ssid = WIFI_SSID; // from creds file
const char* password = WIFI_PASSWD; // from creds file


#define SERIAL_BAUD 115200 //baud rate for Serial debugging

// Sensor inits and constants
String boschType = "uninitialized";
#define DHTPIN D4     // what digital pin the DHT sensor is connected to
#define DHTTYPE DHT22   // Options are DHT11, DHT12, DHT22 (AM2302), DHT21 (AM2301)

Adafruit_BMP280 bmp280; // I2C BMP280 init
Adafruit_BME280 bme280; // I2C BME280 init
DHT dht(DHTPIN, DHTTYPE); // Initialize DHT sensor.

ESP8266WebServer httpServer(80);
ESP8266HTTPUpdateServer httpUpdater;

// method to connect to and initialize whichever Bosch sensor is connected
void setupBosch() {

  unsigned boschStatus;

  // Try BME280 setup
  boschStatus = bme280.begin();
  Serial.print("SensorID was: 0x"); Serial.println(bme280.sensorID(), 16);
  Serial.println("ID of 0xFF could be a bad address, a BMP 180 or BMP 085");
  Serial.println("ID of 0x56-0x58 represents a BMP 280");
  Serial.println("ID of 0x60 represents a BME 280");
  Serial.println("ID of 0x61 represents a BME 680");
  if (!boschStatus) {
    Serial.println("Could not find a valid BME280, check wiring, address, sensor ID!");
    delay(2000);
  } else {
    Serial.println("\nBME280 found!\n");
    boschType = "BME280";
    bme280.setSampling(Adafruit_BME280::MODE_NORMAL, /* Operating Mode. */
                       Adafruit_BME280::SAMPLING_X16, /* Temp. oversampling */
                       Adafruit_BME280::SAMPLING_X16,  /* Pressure oversampling */
                       Adafruit_BME280::SAMPLING_X16, /* Humidity oversampling */
                       Adafruit_BME280::FILTER_OFF, /* Filtering. */
                       Adafruit_BME280::STANDBY_MS_500); /* Standby time. */
    return;
  }

  // Try BMP280 setup
  boschStatus = bmp280.begin(0x76, 0x58); //BMP280 has I2C address 0x76 and chipID of 0x58
  if (!boschStatus) {
    Serial.println("Could not find a valid BMP280, check wiring, address, sensor ID!");
  } else {
    Serial.println("\nBMP280 found!\n");
    boschType = "BMP280";
    /* Default settings from datasheet. */
    bmp280.setSampling(Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */
                       Adafruit_BMP280::SAMPLING_X16,     /* Temp. oversampling */
                       Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
                       Adafruit_BMP280::FILTER_X16,      /* Filtering. */
                       Adafruit_BMP280::STANDBY_MS_500); /* Standby time. */
    return;
  }

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
  dht.begin();
}

void loop(void) {
  httpServer.handleClient();
  if (boschType.equals("BMP280")) {

    Serial.print(F("Temperature = "));
    Serial.print(bmp280.readTemperature());
    Serial.println(" *C");

    Serial.print(F("Pressure = "));
    Serial.print(bmp280.readPressure());
    Serial.println(" Pa");

    Serial.println();
    delay(2000);
  } else if (boschType.equals("BME280")) {
    printValues();
  } else {}

  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  float f = dht.readTemperature(true);

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t) || isnan(f)) {
    Serial.println(F("Failed to read from DHT sensor!"));

  }

  Serial.print(F("Humidity: "));
  Serial.print(h);
  Serial.print(F("%  Temperature: "));
  Serial.print(t);
  Serial.print(F("°C "));
  Serial.println(f);
  Serial.print(F("°F "));
}

void printValues() {
  Serial.print("Temperature = ");
  Serial.print(bme280.readTemperature());
  Serial.println(" *C");

  Serial.print("Pressure = ");

  Serial.print(bme280.readPressure() / 100.0F);
  Serial.println(" hPa");

  Serial.print("Humidity = ");
  Serial.print(bme280.readHumidity());
  Serial.println(" %");

  Serial.println();
  delay(2000);
}
