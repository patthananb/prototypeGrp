#include <Arduino.h>
#include <AiEsp32RotaryEncoder.h>

#define ROTARY_ENCODER_A_PIN        27
#define ROTARY_ENCODER_B_PIN        25
#define ROTARY_ENCODER_BUTTON_PIN   26
#define ROTARY_ENCODER_STEPS        4

// Long press threshold (in milliseconds)
#define LONG_PRESS_TIME 1000 
static const unsigned long DEBOUNCE_DELAY_KNOB = 50;

// Behavior flags
bool rotateLeftFlag       = false;
bool rotateRightFlag      = false;
bool buttonPressedFlag    = false;
bool buttonLongPressedFlag= false;

// Track state
static unsigned long lastKnobChange  = 0;
static long lastEncoderValue         = 0;
static bool lastButtonState          = true;
static unsigned long buttonDownTime  = 0;

// Additional variable to manage repeating long presses
static unsigned long lastLongPressTime = 0;

// Rotary Encoder object
AiEsp32RotaryEncoder rotaryEncoder = 
    AiEsp32RotaryEncoder(ROTARY_ENCODER_A_PIN, ROTARY_ENCODER_B_PIN, ROTARY_ENCODER_STEPS);

void rotary_onButtonClick() {
    Serial.println("Button clicked");
}

// Interrupt Service Routine
void IRAM_ATTR readEncoderISR() {
    rotaryEncoder.readEncoder_ISR();
}

void rotary_loop() {
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
            // Only trigger if at least 1 second has passed since the last long press event
            if ((millis() - lastLongPressTime) > 1000) {
                buttonLongPressedFlag = true;
                lastLongPressTime     = millis();
            }
        }
    }

    // Button released
    if (!currentButtonState && lastButtonState) {
        unsigned long pressDuration = millis() - buttonDownTime;
        // If it was not a long press, treat it as a short press
        if (pressDuration < LONG_PRESS_TIME) {
            buttonPressedFlag = true;
        }
    }
    lastButtonState = currentButtonState;

    // Print and reset flags
    if (rotateRightFlag) {
        Serial.println("Rotated Right");
        rotateRightFlag = false;
    }
    if (rotateLeftFlag) {
        Serial.println("Rotated Left");
        rotateLeftFlag = false;
    }
    if (buttonPressedFlag) {
        Serial.println("Button Pressed");
        buttonPressedFlag = false;
    }
    if (buttonLongPressedFlag) {
        Serial.println("Button Long Pressed");
        buttonLongPressedFlag = false;
    }
}

void setup(){
    Serial.begin(115200);

    // Initialize the rotary encoder
    rotaryEncoder.begin();
    rotaryEncoder.setup([]{ readEncoderISR(); });
    
    // Set boundaries and acceleration
    bool circleValues = true;
    rotaryEncoder.setBoundaries(0, 100, circleValues);
    rotaryEncoder.setAcceleration(250);

    // Ensure the button pin is set as input (pull-up)
    pinMode(ROTARY_ENCODER_BUTTON_PIN, INPUT_PULLUP);

    // Initialize last encoder value
    lastEncoderValue = rotaryEncoder.readEncoder();
    lastButtonState  = true;
}

void loop()
{
    rotary_loop();
    delay(10);
}