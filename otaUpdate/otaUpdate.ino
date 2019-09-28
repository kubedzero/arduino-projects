/*
  To upload through terminal you can use: curl -u admin:admin -F "image=@firmware.bin" <esp-ip-address>/firmware
*/

#include <ESP8266WiFi.h> // ESP8266 specific WiFi library
#include <WiFiClient.h> // library supporting WiFi connection
#include <ESP8266WebServer.h> // library for HTTP Server
#include <ESP8266HTTPUpdateServer.h> // library for update code

#include "myCredentials.h" //used to store WiFi and update credentials

const char* update_path = "/firmware"; // URL path to get to firmware update
const char* update_username = WEB_UPDATE_USER; // from creds file
const char* update_password = WEB_UPDATE_PASS; // from creds file
const char* ssid = WIFI_SSID; // from creds file
const char* password = WIFI_PASSWD; // from creds file

#define SERIAL_BAUD 115200 //baud rate for Serial debugging

ESP8266WebServer httpServer(80);
ESP8266HTTPUpdateServer httpUpdater;

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

  httpUpdater.setup(&httpServer, update_path, update_username, update_password);
  httpServer.begin();

  digitalWrite(LED_BUILTIN, HIGH); //set Blue(GeekCreit) or Red(NodeMCU 0.9) LED to off
}

void loop(void) {
  httpServer.handleClient();
}
