/*
 * SimpleRGBLed - Standalone WS2812 RGB LED driver for ESP32.
 *
 * No external dependencies. Uses the ESP32 RMT peripheral to drive
 * WS2812, WS2812B, WS2811, SK6812, and NeoPixel-compatible LEDs.
 *
 * Features:
 *   - Named colors in English and Spanish (case-insensitive)
 *   - Direct RGB and hex color control
 *   - Brightness adjustment (0-100%)
 *   - Per-channel gamma correction (R 2.2, G 2.8, B 2.5)
 *   - Blink patterns
 *   - Optional FreeRTOS mutex for thread safety
 *
 * License: MIT
 */

#ifndef SIMPLE_RGB_LED_H
#define SIMPLE_RGB_LED_H

#include <Arduino.h>
#include <driver/rmt_tx.h>
#include <driver/rmt_encoder.h>

#ifndef SIMPLE_RGB_MAX_LEDS
#define SIMPLE_RGB_MAX_LEDS 16
#endif

struct RGBColor {
    uint8_t r, g, b;
};

struct NamedColor {
    const char *name;
    RGBColor color;
};

class SimpleRGBLed {
public:
    SimpleRGBLed(uint8_t numLeds = 1);
    ~SimpleRGBLed();

    bool begin(uint8_t pin);

    void setColor(const char *colorName, uint8_t index = 0);
    void setRGB(uint8_t r, uint8_t g, uint8_t b, uint8_t index = 0);
    void setHex(uint32_t hexColor, uint8_t index = 0);

    void setAll(const char *colorName);
    void setAllRGB(uint8_t r, uint8_t g, uint8_t b);
    void setAllHex(uint32_t hexColor);

    void off(uint8_t index = 0);
    void offAll();

    void setBrightnessPercent(uint8_t percent);
    uint8_t getBrightnessPercent() const;

    void setBrightness(uint8_t brightness);
    uint8_t getBrightness() const;

    void enableGammaCorrection(bool enabled = true);
    bool isGammaCorrectionEnabled() const;

    void blink(const char *colorName, uint8_t times = 3,
               uint16_t onMs = 300, uint16_t offMs = 300,
               uint8_t index = 0);

    void enableMutex();

    RGBColor getPixel(uint8_t index) const;
    uint8_t numLeds() const;

    static uint8_t colorCount();
    static const NamedColor *colorAt(uint8_t idx);
    static const NamedColor *findColor(const char *name);

private:
    uint8_t _numLeds;
    uint8_t _brightnessPercent;
    bool _gammaEnabled;
    uint8_t _pin;
    bool _initialized;

    uint8_t _pixelBuf[SIMPLE_RGB_MAX_LEDS * 3]; // GRB order

    rmt_channel_handle_t _rmtChannel;
    rmt_encoder_handle_t _encoder;

    void _applyBrightness(uint8_t r, uint8_t g, uint8_t b, uint8_t *out);
    bool _transmit();

    void _lock();
    void _unlock();
    SemaphoreHandle_t _mutex;

    static bool _createEncoder(rmt_encoder_handle_t *encoder);
    static const NamedColor _colors[];
    static const uint8_t _numColors;
};

#endif
