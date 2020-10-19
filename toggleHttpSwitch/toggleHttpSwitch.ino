#include <ESP8266WiFi.h> // ESP8266 specific WiFi library
#include <WiFiClient.h> // library supporting WiFi connection
#include <ESP8266HTTPClient.h> // library to make HTTP GET calls
#include <ArduinoLog.h> // 1.0.3 library by thijse for outputting different log levels

#include "myCredentials.h" // used to store WiFi credentials

// Log setup
#define SERIAL_BAUD 115200 // baud rate for Serial debugging
// available levels are _SILENT, _FATAL, _ERROR, _WARNING, _NOTICE, _TRACE, _VERBOSE
#define LOG_LEVEL LOG_LEVEL_NOTICE

#define LEDPIN D4 // built in blue LED on the ESP itself
#define INPUT1 D1 // one leg of the toggle switch
#define INPUT2 D2 // the other leg of the toggle switch
#define LAMPRETRYLIMIT 3 // limit the amount of HTTP retries to turn on the lamp
#define TVRETRYLIMIT 2 // limit the amount of HTTP retries to turn on the TV

const char* ssid = WIFI_SSID; // from creds file
const char* password = WIFI_PASSWD; // from creds file

void setup() {

  pinMode(INPUT1, INPUT_PULLUP); // use built-in pullup resistor to keep high when floating
  pinMode(INPUT2, INPUT_PULLUP); // use built-in pullup resistor to keep high when floating
  pinMode(LEDPIN, OUTPUT); // used to visually indicate states

  // Serial, logging, WiFi setup
  Serial.begin(SERIAL_BAUD);
  while (!Serial && !Serial.available()) {}
  delay(200); //add some delay before we start printing
  Serial.println(); // get off the junk line that prints at 74880 baud
  Log.begin(LOG_LEVEL, &Serial);
  Log.setSuffix(printNewline); // put a newline after each log statement
  Log.notice("Booting Sketch...");
  WiFiConnect();

  delay(500);
  // blink on off on off for 1s each to signify bootup
  //blinkWithDelay(1000);

  int input1val = digitalRead(INPUT1); // read the value from the first leg
  int input2val = digitalRead(INPUT2); // read the value from the second leg

    // Check that the second leg is floating (high due to pull-up) 
  // and first leg is LOW (grounded)
  if (!input1val && input2val) {
    //blinkWithDelay(250);
    sendHTTPGET("http://lamp.brad/cm?cmnd=Power%20ON", LAMPRETRYLIMIT);
    // Input 1 is active, trigger the first state
  } else if (!input2val && input1val) {
    //blinkWithDelay(500);
    sendHTTPGET("http://lamp.brad/cm?cmnd=Power%20OFF", LAMPRETRYLIMIT);
    // Input 2 is active, trigger the second state
  } else {
    blinkWithDelay(500);
    // There is some sort of error and both inputs 1 and 2 are HIGH or LOW
  }


  


  // Go to sleep until woken up by a circuit bringing the RST pin LOW
  ESP.deepSleep(0);
}

void loop() {

}


void sendHTTPGET (char * url, int retries) {
  WiFiClient client; // initialize the client we'll use to talk on the network
  HTTPClient http; // initialize the HTTP code the WiFi client will use to speak HTTP
  // set timeout with http.setTimeout(3000);

  

  Serial.print("[HTTP] begin...\n");
  if (http.begin(client, url)) {  // HTTP


    Serial.print("[HTTP] GET...\n");
    // start connection and send HTTP header
    int httpCode = http.GET();

    // httpCode will be negative on error
    if (httpCode > 0) {
      // HTTP header has been send and Server response header has been handled
      Serial.printf("[HTTP] GET... code: %d\n", httpCode);

      // file found at server
      if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
        String payload = http.getString();
        Serial.println(payload);
      }
    } else {
      Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }

    http.end();
  } else {
    Serial.printf("[HTTP} Unable to connect\n");
  }
}


// Connect or reconnect to WiFi network
void WiFiConnect() {
  if (WiFi.status() != WL_CONNECTED)
  {
    digitalWrite(LEDPIN, LOW); // set Blue(GeekCreit) or Red(NodeMCU 0.9) LED to on
    Log.notice("WiFi is not connected. Connecting to %s", ssid);
    WiFi.disconnect();
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
      delay(1000);
      Log.notice("waiting...");
    }
    String ipAddrString = WiFi.localIP().toString(); // convert the IPAddress opject to a String
    char * ipAddrChar = new char [ipAddrString.length() + 1]; // allocate space for a char array
    strcpy (ipAddrChar, ipAddrString.c_str()); // populate the char array
    Log.notice("Connected, IP address is %s", ipAddrChar); // pass the char array to the logger
    digitalWrite(LEDPIN, HIGH); // set Blue(GeekCreit) or Red(NodeMCU 0.9) LED to off
  } else {
    Log.verbose("WiFi is connected, nothing to do");
  }
}

// helper to add a newline at the end of every log statement
void printNewline(Print * _logOutput) {
  _logOutput->print('\n');
}


void blinkWithDelay(int delayMillis) {
  digitalWrite(LEDPIN, !digitalRead(LEDPIN));
  delay(delayMillis);
  digitalWrite(LEDPIN, !digitalRead(LEDPIN));
  delay(delayMillis);
  digitalWrite(LEDPIN, !digitalRead(LEDPIN));
  delay(delayMillis);
  digitalWrite(LEDPIN, !digitalRead(LEDPIN));
  delay(delayMillis);
}
