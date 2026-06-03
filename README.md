# SimpleRGBLed

Standalone WS2812 RGB LED driver for ESP32. **Zero external dependencies** — drives the LEDs directly through the ESP32 RMT peripheral.

## Features

- **No dependencies** — does not require FastLED, Adafruit NeoPixel, or any other library
- **Named colors** — 47 built-in colors in English and Spanish (case-insensitive)
- **Multiple color modes** — set by name, RGB values, or hex code
- **Brightness control** — `setBrightnessPercent(0-100)` with optional per-channel gamma correction (enabled by default)
- **Gamma correction** — per-channel curves (R γ2.2, G γ2.8, B γ2.5) for perceptually uniform colors
- **Blink patterns** — built-in blink function with configurable timing
- **FreeRTOS safe** — optional mutex for multi-task environments
- **Up to 16 LEDs** — configurable via `SIMPLE_RGB_MAX_LEDS`

## Compatible Hardware

| LED Type | Supported |
|----------|-----------|
| WS2812   | Yes       |
| WS2812B  | Yes       |
| WS2811   | Yes       |
| SK6812   | Yes       |
| NeoPixel | Yes       |

**Requires:** Any ESP32 variant (ESP32, ESP32-S2, ESP32-S3, ESP32-C3, ESP32-C6)

## Installation

### Arduino IDE

1. Download this repository as a ZIP file
2. In Arduino IDE: **Sketch → Include Library → Add .ZIP Library**
3. Select the downloaded ZIP

### Manual

Copy the `SimpleRGBLed` folder into your Arduino libraries directory:
- Windows: `Documents\Arduino\libraries\`
- macOS: `~/Documents/Arduino/libraries/`
- Linux: `~/Arduino/libraries/`

## Quick Start

```cpp
#include <SimpleRGBLed.h>

#define LED_PIN  1

SimpleRGBLed led;

void setup() {
    led.begin(LED_PIN);
    led.setBrightnessPercent(50); // 50% brightness
}

void loop() {
    led.setColor("red");      // English
    delay(500);
    led.setColor("verde");    // Spanish
    delay(500);
    led.setRGB(0, 0, 255);   // Direct RGB
    delay(500);
    led.setHex(0xFF8800);    // Hex value
    delay(500);
}
```

## API Reference

### Initialization

| Method | Description |
|--------|-------------|
| `SimpleRGBLed(numLeds)` | Constructor. Default: 1 LED |
| `begin(pin)` | Initialize on GPIO pin. Returns `true` on success |
| `setBrightnessPercent(0-100)` | Set global brightness in percent |
| `getBrightnessPercent()` | Get current brightness percent |
| `setBrightness(0-255)` | Legacy brightness API (maps to percent) |
| `enableGammaCorrection(true/false)` | Enable or disable gamma correction (default: on) |
| `isGammaCorrectionEnabled()` | Check if gamma correction is active |
| `enableMutex()` | Enable FreeRTOS thread safety |

### Setting Colors

| Method | Description |
|--------|-------------|
| `setColor("name", index)` | Set LED by color name |
| `setRGB(r, g, b, index)` | Set LED by RGB values (0-255) |
| `setHex(0xRRGGBB, index)` | Set LED by hex color |
| `setAll("name")` | Set all LEDs to a named color |
| `setAllRGB(r, g, b)` | Set all LEDs to an RGB color |
| `setAllHex(0xRRGGBB)` | Set all LEDs to a hex color |

### Control

| Method | Description |
|--------|-------------|
| `off(index)` | Turn off one LED |
| `offAll()` | Turn off all LEDs |
| `blink("name", times, onMs, offMs, index)` | Blink pattern |

### Info

| Method | Description |
|--------|-------------|
| `getPixel(index)` | Get current RGBColor of a LED |
| `numLeds()` | Get number of LEDs |
| `colorCount()` | Get number of named colors |
| `colorAt(idx)` | Get a NamedColor by index |
| `findColor("name")` | Look up a NamedColor by name |

## Available Colors

### English
`red`, `green`, `blue`, `white`, `black`, `yellow`, `cyan`, `magenta`, `orange`, `pink`, `purple`, `brown`, `violet`, `lilac`, `olive`, `lime`, `navy`, `turquoise`, `salmon`, `gold`, `silver`, `teal`, `coral`, `crimson`, `indigo`

### Spanish
`rojo`, `verde`, `azul`, `blanco`, `negro`, `amarillo`, `cian`, `magenta`, `naranja`, `rosa`, `morado`, `marron`, `violeta`, `lila`, `verde_oliva`, `verde_lima`, `azul_marino`, `turquesa`, `salmon`, `dorado`, `plata`, `coral`, `carmesi`

## FreeRTOS Example

```cpp
SimpleRGBLed led;

void taskA(void *pv) {
    for (;;) {
        led.setColor("green");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void taskB(void *pv) {
    for (;;) {
        led.blink("rojo", 3, 200, 200);
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

void setup() {
    led.begin(1);
    led.enableMutex(); // call before creating tasks
    xTaskCreatePinnedToCore(taskA, "A", 4096, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(taskB, "B", 4096, NULL, 2, NULL, 1);
}

void loop() { vTaskDelay(10000); }
```

## More Than 16 LEDs

Define `SIMPLE_RGB_MAX_LEDS` before including the library:

```cpp
#define SIMPLE_RGB_MAX_LEDS 64
#include <SimpleRGBLed.h>

SimpleRGBLed strip(64);
```

## Gamma Correction (v2.0)

By default, brightness is applied with per-channel gamma correction tuned for WS2812 LEDs:

| Channel | Gamma |
|---------|-------|
| Red     | 2.2   |
| Green   | 2.8   |
| Blue    | 2.5   |

```cpp
led.setBrightnessPercent(15);  // matches production tuning from hardware projects
led.setColor("verde");

led.enableGammaCorrection(false); // linear scaling, v1-style behavior
led.setColor("verde");
```

## How It Works

SimpleRGBLed uses the ESP32's **RMT (Remote Control Transceiver)** peripheral to generate the precise timing signals required by WS2812 LEDs. The RMT hardware handles the bit-level timing in the background, freeing the CPU for other tasks.

## License

MIT License. See [LICENSE](LICENSE) for details.

## Author

**pablojosep** — [github.com/pablojosep](https://github.com/pablojosep)
