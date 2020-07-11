#define FASTLED_ALLOW_INTERRUPTS 0
#include <FastLED.h>


#define LED_DATA_PIN 3 // data pin of the LED strip, requried for WS2812, WS2801, etc.
#define LED_CLOCK_PIN 2 // clock pin of SPI-based chipsets such as WS2801

#define NUM_LEDS 85 // total number of LEDs in the connected strip/strand
#define LED_MAX_BRIGHTNESS 255 // 0-255 value of max brightness, 10 is safe for testing

#define CHIPSET WS2812 // ENUM of a supported FastLED chipset (WS2812, WS2801, etc.)
#define CHIPSET_COLOR_ORDER GRB // RGB (WS2801), GRB (typical WS2812) or another color order ENUM
#define LED_CORRECTION_TYPE CRGB(200, 255, 255) // Pre-calibrated RGB bias to account for LED quirks. TypicalSMD5050 or TypicalPixelString

#define LED_TEMPERATURE DirectSunlight // ENUM from color.h to configure the temperature of "white" 255,255,255

#define FRAMES_PER_SECOND 24 // Number of updates to the strip sent per second, aka the refresh rate or FPS

CRGB leds[NUM_LEDS]; // initialize the LED strip variable


// run-once startup information
void setup() {

  // initialize either using SPI (clock pin) or clockless
  FastLED.addLeds<CHIPSET, LED_DATA_PIN, CHIPSET_COLOR_ORDER>(leds, NUM_LEDS).setCorrection( LED_CORRECTION_TYPE );
  //FastLED.addLeds<CHIPSET, LED_DATA_PIN, LED_CLOCK_PIN, CHIPSET_COLOR_ORDER>(leds, NUM_LEDS).setCorrection( LED_CORRECTION_TYPE );

  // set the max allowed brightness of the strip


  // set the color calibration in Kelvin
  FastLED.setTemperature( LED_TEMPERATURE );

  // fade the LEDs in slowly, from min brightness to max brightness
  for ( int brightStep = 0; brightStep < LED_MAX_BRIGHTNESS; brightStep++ ) {
    FastLED.setBrightness( brightStep );
    fill_solid(leds, NUM_LEDS, CRGB::White);
    FastLED.show();
    FastLED.delay(1000 / FRAMES_PER_SECOND);
  }

}


// run repeatedly, the speed of this loop determines the FPS/refresh rate/update speed
void loop() {
  fill_solid(leds, NUM_LEDS, CRGB::White);
  FastLED.show();
  // Use the FastLED version of delay to slow down the loop speed
  FastLED.delay(1000 / FRAMES_PER_SECOND);
}
