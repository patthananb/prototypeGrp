#include <Arduino.h>
#include <EEPROM.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <AiEsp32RotaryEncoder.h>

#define ROTARY_ENCODER_A_PIN 27
#define ROTARY_ENCODER_B_PIN 25
#define ROTARY_ENCODER_BUTTON_PIN 26
#define ROTARY_ENCODER_STEPS 4

#define EEPROM_SIZE 512
#define EEPROM_ADDRESS 0    

#define DEBOUNCE_DELAY_KNOB 50
#define LONG_PRESS_TIME 1000

int intensity[5] = {0, 0, 0, 0, 0}; // Initialize intensity array

AiEsp32RotaryEncoder rotaryEncoder = AiEsp32RotaryEncoder(ROTARY_ENCODER_A_PIN, ROTARY_ENCODER_B_PIN, ROTARY_ENCODER_STEPS);

void IRAM_ATTR readEncoderISR() {
    rotaryEncoder.readEncoder_ISR();
}

// Declare variables for debounce and long press detection
unsigned long lastKnobChange = 0;
unsigned long lastEncoderValue = 0;
unsigned long buttonDownTime = 0;
unsigned long lastLongPressTime = 0;
unsigned long lastActivityTime = 0;
bool lastButtonState = false;
bool rotateRightFlag = false;
bool rotateLeftFlag = false;
bool buttonPressFlag = false;
bool buttonHeldFlag = false;

void rotaryTask(void *pvParameters) {
    for (;;) {
        // 1) Check for rotation (debounced)
        if (rotaryEncoder.encoderChanged()) {
            unsigned long currentTime = millis();
            if (currentTime - lastKnobChange > DEBOUNCE_DELAY_KNOB) {
                lastKnobChange = currentTime;

                long currentEncoderValue = rotaryEncoder.readEncoder();
                if (currentEncoderValue > lastEncoderValue) {
                    rotateRightFlag = true;
                    //Serial.println("Rotated Right");
                } else if (currentEncoderValue < lastEncoderValue) {
                    rotateLeftFlag = true;
                    //Serial.println("Rotated Left");
                }
                lastEncoderValue = currentEncoderValue;
                lastActivityTime = millis(); // Reset inactivity timer
            }
        }

        // 2) Check for button press and long press
        bool currentButtonState = (digitalRead(ROTARY_ENCODER_BUTTON_PIN) == LOW);

        // Button went down
        if (currentButtonState && !lastButtonState) {
            buttonDownTime = millis();
            lastLongPressTime = 0; // reset when freshly pressed
        }

        // Button is held down
        if (currentButtonState) {
            // Check if we've passed the initial long press time
            if ((millis() - buttonDownTime) > LONG_PRESS_TIME) {
                // Only trigger if at least 1 second has passed since last long press event
                if ((millis() - lastLongPressTime) > 1000) {
                    buttonHeldFlag = true;
                    //Serial.println("Button Held");
                    lastLongPressTime = millis();
                    lastActivityTime = millis(); // Reset inactivity timer
                }
            }
        }

        // Button released
        if (!currentButtonState && lastButtonState) {
            unsigned long pressDuration = millis() - buttonDownTime;
            // If it was not a long press, treat it as a short press
            if (pressDuration < LONG_PRESS_TIME) {
                buttonPressFlag = true;
                //Serial.println("Button Pressed");
                lastActivityTime = millis(); // Reset inactivity timer
            }
            buttonHeldFlag = false;
        }
        lastButtonState = currentButtonState;

        // Avoid hogging the CPU
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void setup() {
    Serial.begin(115200);
    EEPROM.begin(EEPROM_SIZE);

    // Initialize the rotary encoder
    rotaryEncoder.begin();
    rotaryEncoder.setup([]{ readEncoderISR(); });
    rotaryEncoder.setBoundaries(0, 100, true); // Set boundaries for encoder value
    rotaryEncoder.setAcceleration(0);

    // Ensure the button pin is set as input (pull-up)
    pinMode(ROTARY_ENCODER_BUTTON_PIN, INPUT_PULLUP);

    // Create FreeRTOS task for rotary encoder
    xTaskCreate(rotaryTask, "RotaryTask", 2048, NULL, 1, NULL);

    // Read initial values from EEPROM
    readArrayFromEEPROM(intensity, 5);

    // Set initial encoder value from EEPROM
    rotaryEncoder.setEncoderValue(intensity[1]);
}

void loop() {
    // Check for serial input to print EEPROM values
    if (Serial.available()) {
        String command = Serial.readStringUntil('\n');
        if (command == "print") {
            Serial.println("EEPROM Values:");
            for (int i = 0; i < 5; i++) {
                Serial.print("Intensity[");
                Serial.print(i);
                Serial.print("]: ");
                Serial.println(EEPROM.read(EEPROM_ADDRESS + i));
            }
        }
    }

    // Modify intensity[1] based on encoder rotation
    if (rotateRightFlag || rotateLeftFlag) {
        intensity[1] = constrain(rotaryEncoder.readEncoder(), 0, 100);
        Serial.print("Intensity[1]: ");
        Serial.println(intensity[1]);
        rotateRightFlag = false;
        rotateLeftFlag = false;
    }

    // Store intensity[1] in EEPROM when button is pressed
    if (buttonPressFlag) {
        EEPROM.write(EEPROM_ADDRESS + 1, intensity[1]);
        EEPROM.commit();
        Serial.println("Intensity[1] stored in EEPROM");
        buttonPressFlag = false;
    }

    // Main loop can be used for other tasks or left empty
    delay(1000);
}

void writeArrayToEEPROM(int arr[], int size) {
    for (int i = 0; i < size; i++) {
        EEPROM.write(EEPROM_ADDRESS + i, arr[i]);
    }
    EEPROM.commit();
    Serial.println("Array stored in EEPROM");
}

void readArrayFromEEPROM(int arr[], int size) {
    for (int i = 0; i < size; i++) {
        arr[i] = EEPROM.read(EEPROM_ADDRESS + i);
    }
    Serial.println("Array read from EEPROM");
    for (int i = 0; i < size; i++) {
        Serial.print(arr[i]);
        Serial.print(" ");
    }
    Serial.println();
}