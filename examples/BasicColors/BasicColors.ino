/*
 * BasicColors - SimpleRGBLed example
 *
 * Cycles through named colors on a single WS2812 LED.
 * Works on any ESP32 board (ESP32, ESP32-S2, ESP32-S3, ESP32-C3, etc.)
 */

#include <SimpleRGBLed.h>

#define LED_PIN  1
#define NUM_LEDS 1

SimpleRGBLed led(NUM_LEDS);

void setup() {
    Serial.begin(115200);
    delay(1000);

    if (!led.begin(LED_PIN)) {
        Serial.println("Error: could not initialize LED");
        while (true) delay(1000);
    }

    led.setBrightness(128); // 50% brightness
    Serial.println("SimpleRGBLed ready!");
}

void loop() {
    // Set color by name (English)
    led.setColor("red");
    delay(500);

    led.setColor("green");
    delay(500);

    led.setColor("blue");
    delay(500);

    // Set color by name (Spanish)
    led.setColor("naranja");
    delay(500);

    led.setColor("morado");
    delay(500);

    led.setColor("turquesa");
    delay(500);

    // Set color by RGB values
    led.setRGB(255, 100, 0);
    delay(500);

    // Set color by hex
    led.setHex(0x00FF88);
    delay(500);

    // Blink 3 times in yellow
    led.blink("amarillo", 3, 200, 200);
    delay(1000);

    // List all available colors
    Serial.printf("Available colors: %d\n", led.colorCount());
    for (uint8_t i = 0; i < led.colorCount(); i++) {
        const NamedColor *c = led.colorAt(i);
        Serial.printf("  %-15s (%3d, %3d, %3d)\n",
                       c->name, c->color.r, c->color.g, c->color.b);
    }
    Serial.println();

    led.offAll();
    delay(2000);
}
