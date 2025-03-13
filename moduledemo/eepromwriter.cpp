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

enum State
{
    selectChannel,
    adjustChannel1,
    adjustChannel2,
    adjustChannel3,
};

State currentState = selectChannel;

int intensity[2]; // Initialize intensity array
int pageIndex = 0;
AiEsp32RotaryEncoder rotaryEncoder = AiEsp32RotaryEncoder(ROTARY_ENCODER_A_PIN, ROTARY_ENCODER_B_PIN, ROTARY_ENCODER_STEPS);

void IRAM_ATTR readEncoderISR()
{
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

void resetflag();


void rotaryTask(void *pvParameters)
{
    for (;;)
    {
        // 1) Check for rotation (debounced)
        if (rotaryEncoder.encoderChanged())
        {
            unsigned long currentTime = millis();
            if (currentTime - lastKnobChange > DEBOUNCE_DELAY_KNOB)
            {
                lastKnobChange = currentTime;

                long currentEncoderValue = rotaryEncoder.readEncoder();
                if (currentEncoderValue > lastEncoderValue)
                {
                    rotateRightFlag = true;
                    // Serial.println("Rotated Right");
                }
                else if (currentEncoderValue < lastEncoderValue)
                {
                    rotateLeftFlag = true;
                    // Serial.println("Rotated Left");
                }
                lastEncoderValue = currentEncoderValue;
                lastActivityTime = millis(); // Reset inactivity timer
            }
        }

        // 2) Check for button press and long press
        bool currentButtonState = (digitalRead(ROTARY_ENCODER_BUTTON_PIN) == LOW);

        // Button went down
        if (currentButtonState && !lastButtonState)
        {
            buttonDownTime = millis();
            lastLongPressTime = 0; // reset when freshly pressed
        }

        // Button is held down
        if (currentButtonState)
        {
            // Check if we've passed the initial long press time
            if ((millis() - buttonDownTime) > LONG_PRESS_TIME)
            {
                // Only trigger if at least 1 second has passed since last long press event
                if ((millis() - lastLongPressTime) > 1000)
                {
                    buttonHeldFlag = true;
                    // Serial.println("Button Held");
                    lastLongPressTime = millis();
                    lastActivityTime = millis(); // Reset inactivity timer
                }
            }
        }

        // Button released
        if (!currentButtonState && lastButtonState)
        {
            unsigned long pressDuration = millis() - buttonDownTime;
            // If it was not a long press, treat it as a short press
            if (pressDuration < LONG_PRESS_TIME)
            {
                buttonPressFlag = true;
                // Serial.println("Button Pressed");
                lastActivityTime = millis(); // Reset inactivity timer
            }
            buttonHeldFlag = false;
        }
        lastButtonState = currentButtonState;

        // Avoid hogging the CPU
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void setup()
{
    Serial.begin(115200);
    EEPROM.begin(EEPROM_SIZE);

    // Initialize the rotary encoder
    rotaryEncoder.begin();
    rotaryEncoder.setup([]
                        { readEncoderISR(); });
    rotaryEncoder.setBoundaries(0, 100, true); // Set boundaries for encoder value
    rotaryEncoder.setAcceleration(0);

    // Ensure the button pin is set as input (pull-up)
    pinMode(ROTARY_ENCODER_BUTTON_PIN, INPUT_PULLUP);

    // Create FreeRTOS task for rotary encoder
    xTaskCreate(rotaryTask, "RotaryTask", 2048, NULL, 1, NULL);

    // Read initial values from EEPROM
    readArrayFromEEPROM(intensity, 2);

    // Set initial encoder value from EEPROM
    rotaryEncoder.setEncoderValue(intensity[1]);
}

void loop()
{
    switch (currentState)
    {
    case selectChannel:
    {
        State currentStateTemp = adjustChannel1;
        int ngo = 0;
        Serial.println("selectChannel ma leaw");
        while (ngo == 0)
        {
            if (rotateLeftFlag)
            {
                pageIndex++;
                if (pageIndex % 3 == 0)
                {
                    currentStateTemp = adjustChannel1;
                }
                else if (pageIndex % 3 == 1)
                {
                    currentStateTemp = adjustChannel2;
                }
                else if (pageIndex % 3 == 2)
                {
                    currentStateTemp = adjustChannel3;
                }
                else if (rotateRightFlag)
                {
                    pageIndex--;
                    if (pageIndex % 3 == 0)
                    {
                        currentStateTemp = adjustChannel1;
                    }
                    else if (pageIndex % 3 == 1)
                    {
                        currentStateTemp = adjustChannel2;
                    }
                    else if (pageIndex % 3 == 2)
                    {
                        currentStateTemp = adjustChannel3;
                    }
                    else if (buttonPressFlag)
                    {
                        ngo = 1;
                        currentState = currentStateTemp;
                        Serial.println(currentState);
                        pageIndex = 0; // reset page index
                        resetflag();
                    }
                }
            }
        case adjustChannel1:
        {
            Serial.println("adjustChannel1 ma leaw");
        }
        case adjustChannel2:
        {
            Serial.println("adjustChannel2 ma leaw");
        }
        case adjustChannel3:
        {
            Serial.println("adjustChannel3 ma leaw");
        }
        }
        delay(500);
    }

        void writeArrayToEEPROM(int arr[], int size)
        {
            for (int i = 0; i < size; i++)
            {
                EEPROM.write(EEPROM_ADDRESS + i, arr[i]);
            }
            EEPROM.commit();
            Serial.println("Array stored in EEPROM");
        }

        void readArrayFromEEPROM(int arr[], int size)
        {
            for (int i = 0; i < size; i++)
            {
                arr[i] = EEPROM.read(EEPROM_ADDRESS + i);
            }
            Serial.println("Array read from EEPROM");
            for (int i = 0; i < size; i++)
            {
                Serial.print(arr[i]);
                Serial.print(" ");
            }
            Serial.println();
        }
        void resetflag()
        {
            rotateRightFlag = false;
            rotateLeftFlag = false;
            buttonPressFlag = false;
            buttonHeldFlag = false;
        }