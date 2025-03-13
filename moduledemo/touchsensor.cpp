#include <Arduino.h>

#define TOUCH_PIN 0  // Define the touch sensor pin

void setup() {
    Serial.begin(115200);  // Initialize serial communication
    touchAttachInterrupt(TOUCH_PIN, onTouch, 40);  // Attach touch sensor interrupt
}

void loop() {
    // Main loop does nothing, touch sensor is handled by interrupt
}

void onTouch() {
    Serial.println("Touch detected!");  // Print message when touch is detected
}