#include <Arduino.h>
#include <AiEsp32RotaryEncoder.h>
#include <LiquidCrystal_I2C.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

LiquidCrystal_I2C lcd(0x27, 16, 2);

#define ROTARY_ENCODER_A_PIN        27
#define ROTARY_ENCODER_B_PIN        25
#define ROTARY_ENCODER_BUTTON_PIN   26
#define ROTARY_ENCODER_STEPS        4

// Long press threshold (in milliseconds)
#define LONG_PRESS_TIME 1000 
static const unsigned long DEBOUNCE_DELAY_KNOB = 50;
static const unsigned long INACTIVITY_TIMEOUT = 3000; // 3 seconds

// Behavior flags
bool rotateLeftFlag       = false;
bool rotateRightFlag      = false;
bool buttonPressedFlag    = false;
bool buttonLongPressedFlag= false;
bool buttonHeldFlag       = false;

// Track state
static unsigned long lastKnobChange  = 0;
static long lastEncoderValue         = 0;
static bool lastButtonState          = true;
static unsigned long buttonDownTime  = 0;
static unsigned long lastLongPressTime = 0;
static unsigned long lastActivityTime = 0;

int channel = 1;
int onoff = 0; //off = 0, on =1
int intensity = 0;
char timerValue;

static int currentPage = 0;
static int totalPages = 3; // Number of pages to cycle through
static int selectedChannel = 0; // Track selected channel

enum State {
    HOME,
    SELECT_CHANNEL,
    ADJUST_INTENSITY
};

State currentState = HOME;

//declare lcd functions prototype
void lcdHomepage();
void selectChannel();
void onofftimer();
void selectedChannel1();
void selectedChannel2();
void selectedChannel3();
void selectedChannel4();
void selectedChannel5();
void onTimer();
void offTimer();

// Rotary Encoder object
AiEsp32RotaryEncoder rotaryEncoder = 
    AiEsp32RotaryEncoder(ROTARY_ENCODER_A_PIN, ROTARY_ENCODER_B_PIN, ROTARY_ENCODER_STEPS);

void IRAM_ATTR readEncoderISR() {
    rotaryEncoder.readEncoder_ISR();
}

void rotary_onButtonClick() {
    Serial.println("Button clicked");
}

// FreeRTOS task function for rotary encoder
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
                } else if (currentEncoderValue < lastEncoderValue) {
                    rotateLeftFlag = true;
                }
                lastEncoderValue = currentEncoderValue;
                lastActivityTime = millis(); // Reset inactivity timer
            }
        }

        // 2) Check for button press and long press
        bool currentButtonState = (digitalRead(ROTARY_ENCODER_BUTTON_PIN) == LOW);

        // Button went down
        if (currentButtonState && !lastButtonState) {
            buttonDownTime     = millis();
            lastLongPressTime  = 0; // reset when freshly pressed
        }

        // Button is held down
        if (currentButtonState) {
            // Check if we've passed the initial long press time
            if ((millis() - buttonDownTime) > LONG_PRESS_TIME) {
                // Only trigger if at least 1 second has passed since last long press event
                if ((millis() - lastLongPressTime) > 1000) {
                    buttonLongPressedFlag = true;
                    buttonHeldFlag = true;
                    lastLongPressTime     = millis();
                    lastActivityTime = millis(); // Reset inactivity timer
                }
            }
        }

        // Button released
        if (!currentButtonState && lastButtonState) {
            unsigned long pressDuration = millis() - buttonDownTime;
            // If it was not a long press, treat it as a short press
            if (pressDuration < LONG_PRESS_TIME) {
                buttonPressedFlag = true;
                lastActivityTime = millis(); // Reset inactivity timer
            }
            buttonHeldFlag = false;
        }
        lastButtonState = currentButtonState;

        // Avoid hogging the CPU
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// FreeRTOS task function to print current page
void printCurrentPageTask(void *pvParameters) {
    for (;;) {
        Serial.print("Current Page: ");
        Serial.println(currentPage);
        vTaskDelay(pdMS_TO_TICKS(1000)); // Print every second
    }
}

void setup() {
    Serial.begin(115200);

    pinMode(ROTARY_ENCODER_BUTTON_PIN, INPUT_PULLUP);

    // Initialize the rotary encoder
    rotaryEncoder.begin();
    rotaryEncoder.setup([]{ readEncoderISR(); });
    rotaryEncoder.setBoundaries(0, 100, true);

    // Disable acceleration
    rotaryEncoder.setAcceleration(0);

    // Initialize last encoder value
    lastEncoderValue = rotaryEncoder.readEncoder();
    lastButtonState  = true;

    // Create the FreeRTOS tasks
    xTaskCreate(rotaryTask, "RotaryTask", 2048, NULL, 1, NULL);
    xTaskCreate(printCurrentPageTask, "PrintCurrentPageTask", 2048, NULL, 1, NULL);

    // Initialize the LCD
    lcd.init();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("WELCOME");
    lcd.setCursor(0, 1);
    lcd.print("Initializing...");
    delay(2000);
}

void loop() {
    unsigned long currentTime = millis();

    switch (currentState) {
        case HOME:
            lcdHomepage();
            if (buttonHeldFlag) {
                currentState = SELECT_CHANNEL;
                lastActivityTime = currentTime;
            }
            break;

        case SELECT_CHANNEL:
            selectChannel();
            if (buttonPressedFlag) {
                currentState = ADJUST_INTENSITY;
                buttonPressedFlag = false;
                lastActivityTime = currentTime;
            } else if (currentTime - lastActivityTime > INACTIVITY_TIMEOUT) {
                currentState = HOME;
            }
            break;

        case ADJUST_INTENSITY:
            switch (selectedChannel) {
                case 0:
                    selectedChannel1();
                    break;
                case 1:
                    selectedChannel2();
                    break;
                case 2:
                    selectedChannel3();
                    break;
                case 3:
                    selectedChannel4();
                    break;
                case 4:
                    selectedChannel5();
                    break;
            }
            if (buttonPressedFlag) {
                buttonPressedFlag = false;
                // Adjust intensity logic here
                intensity++;
                if (intensity > 100) intensity = 0;
                lastActivityTime = currentTime;
            } else if (rotateRightFlag) {
                selectedChannel = (selectedChannel + 1) % 5;
                rotateRightFlag = false;
                lastActivityTime = currentTime;
            } else if (rotateLeftFlag) {
                selectedChannel = (selectedChannel - 1 + 5) % 5;
                rotateLeftFlag = false;
                lastActivityTime = currentTime;
            } else if (currentTime - lastActivityTime > INACTIVITY_TIMEOUT) {
                currentState = HOME;
            }
            break;
    }

    // Avoid hogging the CPU
    delay(100);
}

void lcdHomepage(){
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("HOMEPAGE");
    lcd.setCursor(0, 1);
    lcd.print("Channel: ");
    lcd.print(channel);
}

// Layer 2
void selectChannel(){
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("SELECT CHANNEL");
    lcd.setCursor(0, 1);
    lcd.print("Channel: ");
    lcd.print(channel);
}

void onofftimer(){
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("ON/OFF TIMER");
    lcd.setCursor(0, 1);
    lcd.print(onoff);
}

// Layer 3
// Select channel
void selectedChannel1(){
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Channel 1");
    lcd.setCursor(0, 1);
    lcd.print("Intensity: ");
    lcd.print(intensity);
}
void selectedChannel2(){
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Channel 2");
    lcd.setCursor(0, 1);
    lcd.print("Intensity: ");
    lcd.print(intensity);
}
void selectedChannel3(){
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Channel 3");
    lcd.setCursor(0, 1);
    lcd.print("Intensity: ");
    lcd.print(intensity);
}
void selectedChannel4(){
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Channel 4");
    lcd.setCursor(0, 1);
    lcd.print("Intensity: ");
    lcd.print(intensity);
}
void selectedChannel5(){
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Channel 5");
    lcd.setCursor(0, 1);
    lcd.print("Intensity: ");
    lcd.print(intensity);
}