/*
 * FreeRTOS_Safe - SimpleRGBLed thread-safe example
 *
 * Two FreeRTOS tasks control the same LED safely using the built-in mutex.
 * Task A sets colors by name, Task B blinks on alerts.
 */

#include <SimpleRGBLed.h>

#define LED_PIN  1
#define NUM_LEDS 1

SimpleRGBLed led(NUM_LEDS);

void taskStatus(void *pv) {
    const char *colors[] = {"green", "cyan", "blue", "verde_lima"};
    uint8_t idx = 0;
    for (;;) {
        led.setColor(colors[idx]);
        idx = (idx + 1) % 4;
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

void taskAlert(void *pv) {
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(7000));
        led.blink("rojo", 5, 150, 150);
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    if (!led.begin(LED_PIN)) {
        Serial.println("Error: could not initialize LED");
        while (true) delay(1000);
    }

    led.setBrightness(100);
    led.enableMutex(); // enable thread safety before creating tasks

    xTaskCreatePinnedToCore(taskStatus, "Status", 4096, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(taskAlert,  "Alert",  4096, NULL, 2, NULL, 1);

    Serial.println("FreeRTOS tasks started");
}

void loop() {
    vTaskDelay(pdMS_TO_TICKS(10000));
}
