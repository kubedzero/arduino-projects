#include <ESP8266WiFi.h> // 2.7.4 ESP8266 specific WiFi library
#include <WiFiClient.h> // library supporting WiFi connection
#include <ESP8266HTTPClient.h> // library to make HTTP GET calls
#include <ArduinoLog.h> // 1.0.3 library by thijse for outputting different log levels

#include "myCredentials.h" // used to store WiFi credentials

// Log setup
#define SERIAL_BAUD 115200 // baud rate for Serial debugging
// available levels are _SILENT, _FATAL, _ERROR, _WARNING, _NOTICE, _TRACE, _VERBOSE
#define LOG_LEVEL LOG_LEVEL_NOTICE

// I/O variables and constants
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
  Serial.println(); // get off the junk line that prints at 74880 baud
  Log.begin(LOG_LEVEL, &Serial);
  Log.setSuffix(printNewline); // put a newline after each log statement
  Log.notice("Booting Sketch...");
  WiFiConnect(); // run the helper that connects to WiFi

  int input1val = digitalRead(INPUT1); // read the value from the first leg
  int input2val = digitalRead(INPUT2); // read the value from the second leg

  // Check which input is connected (pulled up by default, LOW if connected to GND)
  if (!input1val && input2val) {
    Log.notice("Input 1 (ON) is connected");
    // Cast as char * to avoid: `warning: deprecated conversion from string constant to 'char*'`
    sendHTTPGET((char *)"http://lamp.brad/cm?cmnd=Power%20ON", LAMPRETRYLIMIT);
    //sendHTTPGET((char *)"http://tv.brad/cm?cmnd=Power%20ON", TVRETRYLIMIT);
  } else if (!input2val && input1val) {
    Log.notice("Input 2 (OFF) is connected");
    // Cast as char * to avoid: `warning: deprecated conversion from string constant to 'char*'`
    sendHTTPGET((char *)"http://lamp.brad/cm?cmnd=Power%20OFF", LAMPRETRYLIMIT);
    //sendHTTPGET((char *)"http://tv.brad/cm?cmnd=Power%20OFF", TVRETRYLIMIT);
  } else {
    Log.notice("Neither or both inputs are connected, skipping HTTP calls");
  }

  Log.notice("Actions complete, sleeping until woken up by a circuit bringing the RST pin LOW");
  ESP.deepSleep(0);
}

// Given a URL and a number of attempts to make, perform an HTTP GET call
void sendHTTPGET (char * url, int attempts) {
  for (int i = 0; i <= attempts; i++) {
    Log.notice("Initiating attempt %d of URL %s", i, url);
    WiFiClient client; // initialize the client we'll use to talk on the network
    HTTPClient http; // initialize the code the WiFi client will use to speak HTTP
    http.setTimeout(3000);
    if (http.begin(client, url)) {  // Establish the HTTP connection
      int httpCode = http.GET(); // send the GET request
      // httpCode will be negative on error, positive otherwise
      if (httpCode > 0) {
        Log.notice("Server HTTP GET returned with code: %d", httpCode);
        if (httpCode == HTTP_CODE_OK) {
          Log.notice(http.getString().c_str()); // print the payload
          break; // end the for loop early since we succeeded
        }
      } else {
        Log.warning("Server HTTP GET returned error: %s", http.errorToString(httpCode).c_str());
      }
      http.end(); // close the HTTP client cleanly
    } else {
      Log.warning("Server HTTP GET was unable to connect on attempt %d", i);
    }
  }
  Log.warning("No attempts remain for the call to %s, moving on", url);
}


// Connect or reconnect to WiFi network. LED stays ON for this step
void WiFiConnect() {
  if (WiFi.status() != WL_CONNECTED) {
    digitalWrite(LEDPIN, LOW); // set the LED ON by bringing the pin LOW
    Log.notice("WiFi is not connected. Connecting to %s", ssid);
    WiFi.disconnect();
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
      delay(250);
      Log.notice("waiting...");
    }
    // Get the IP as a String, then a Char array, and log it
    Log.notice("Connected, IP address is %s", WiFi.localIP().toString().c_str()); 
    digitalWrite(LEDPIN, HIGH); // set the LED OFF by bringing the pin HIGH
  } else {
    Log.notice("WiFi is already connected, nothing to do");
  }
}

// Helper to add a newline at the end of every log statement
void printNewline(Print * _logOutput) {
  _logOutput->print('\n');
}

// We boot up each time something needs to be done, so no looping action is necessary
void loop() {
}
