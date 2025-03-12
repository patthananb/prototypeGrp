#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <LiquidCrystal_I2C.h>
#include <AiEsp32RotaryEncoder.h>
#include <EEPROM.h>
// #define TOUCH_PIN_1 32 // GPIO 32
// #define TOUCH_PIN_2 33 // GPIO 33
// #define TOUCH_PIN_3 12 // GPIO 12
// #define TOUCH_PIN_4 14 // GPIO 14

#define EEPROM_SIZE 512
#define EEPROM_ADDRESS 0

#define ROTARY_ENCODER_A_PIN 27
#define ROTARY_ENCODER_B_PIN 25
#define ROTARY_ENCODER_BUTTON_PIN 26
#define ROTARY_ENCODER_STEPS 4


// Track state
static unsigned long lastKnobChange  = 0;
static long lastEncoderValue         = 0;
static bool lastButtonState          = true;
static unsigned long buttonDownTime  = 0;
static unsigned long lastLongPressTime = 0;
static unsigned long lastActivityTime = 0;

#define LONG_PRESS_TIME 1000
static const unsigned long DEBOUNCE_DELAY_KNOB = 50;
static const unsigned long INACTIVITY_TIMEOUT = 10000;

// Set a threshold based on your board's sensitivity

//#define TOUCH_THRESHOLD 30

// Behavior flags
bool rotateRightFlag = false;
bool rotateLeftFlag = false;
bool buttonPressFlag = false;
bool buttonHeldFlag = false;

int pageIndex = 0;
int intensity[5];

//lcd functions protptype
void lcdHomepage();
void selectChannel();
void onofftimer();

void selectingChannel1();
void selectingChannel2();
void selectingChannel3();
void selectingChannel4();
void selectingChannel5();

void selectedChannel1();
void selectedChannel2();
void selectedChannel3();
void selectedChannel4();
void selectedChannel5();
void resetflag();

void writeArrayToEEPROM(int arr[], int size);
void readArrayFromEEPROM(int arr[], int size);

enum State
{
    HOME,
    MODE_SELECT,
    SELECT_CHANNEL_MODE,
    ON_OFF_TIMER_MODE,
    ADJUST_INTENSITY__CH1_MODE,
    ADJUST_INTENSITY__CH2_MODE,
    ADJUST_INTENSITY__CH3_MODE,
    ADJUST_INTENSITY__CH4_MODE,
    ADJUST_INTENSITY__CH5_MODE,
};

State currentState = HOME;

LiquidCrystal_I2C lcd(0x27, 16, 2);

void touchTask(void *pvParameters);

AiEsp32RotaryEncoder rotaryEncoder = AiEsp32RotaryEncoder(ROTARY_ENCODER_A_PIN, ROTARY_ENCODER_B_PIN, ROTARY_ENCODER_STEPS);

void IRAM_ATTR readEncoderISR()
{
    rotaryEncoder.readEncoder_ISR();
}

void rotaryTask(void *pvParameters){
    for (;;) {
        // 1) Check for rotation (debounced)
        if (rotaryEncoder.encoderChanged()) {
            unsigned long currentTime = millis();
            if (currentTime - lastKnobChange > DEBOUNCE_DELAY_KNOB) {
                lastKnobChange = currentTime;

                long currentEncoderValue = rotaryEncoder.readEncoder();
                if (currentEncoderValue > lastEncoderValue) {
                    rotateRightFlag = true;
                    Serial.println("Rotated Right");
                } else if (currentEncoderValue < lastEncoderValue) {
                    rotateLeftFlag = true;
                    Serial.println("Rotated Left");
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
                    buttonHeldFlag = true;
                    Serial.println("Button Held");
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
                buttonPressFlag = true;
                Serial.println("Button Pressed");
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
    Serial.println("ESPino32 Multi-Touch Sensor Ready!");
    pinMode(ROTARY_ENCODER_BUTTON_PIN, INPUT_PULLUP);
    resetflag();
    rotaryEncoder.begin();
    rotaryEncoder.setup([] { readEncoderISR(); });
    rotaryEncoder.setBoundaries(1, 100000, true);

    rotaryEncoder.setAcceleration(0);
    lastEncoderValue = rotaryEncoder.readEncoder();
    lastButtonState = true;
    xTaskCreate(rotaryTask, "RotaryTask", 2048, NULL, 1, NULL);
    // Create FreeRTOS tasks
    //xTaskCreate(touchTask, "TouchTask", 2048, NULL, 1, NULL);
    lcd.init();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("WELCOME");
    lcd.setCursor(0, 1);
    lcd.print("Initializing...");
    delay(500);
    currentState = ADJUST_INTENSITY__CH1_MODE;
}

void loop()
{
    if(Serial.available()){
        String command = Serial.readStringUntil('\n');
        if (command == "printinten")
        {
            Serial.println("EEPROM Values:");
            for (int i = 0; i < 5; i++)
            {
                Serial.print("Intensity[");
                Serial.print(i);
                Serial.print("]: ");
                Serial.println(intensity[i]);
                Serial.println(EEPROM.read(EEPROM_ADDRESS + i));
            }
        }
        
    }
    // for rotary encoder debugging
    // if(rotateLeftFlag){
    //     Serial.println("Rotated Left");
    //     resetflag();
    // }
    // else if(rotateRightFlag){
    //     Serial.println("Rotated Right");
    //     resetflag();
    // }
    // else if(buttonPressFlag){
    //     Serial.println("Button Pressed");
    //     resetflag();
    // }
    // else if(buttonHeldFlag){
    //     Serial.println("Button Held");
    //     resetflag();
    // }
    switch (currentState)
    {
    case HOME:
    {
        Serial.println("HOME malaew");
        // display homepage
        lcdHomepage();
        if (buttonHeldFlag == true)
        {
            // Serial.println("button held ma if laew jing jing na");
            currentState = MODE_SELECT;
            resetflag();
        }
    }
    break;
    case MODE_SELECT:
    {
        State currentStateTemp = SELECT_CHANNEL_MODE;
        selectChannel();
        int ngo = 0;
        Serial.println("MODE_SELECT ma leaw");
        while (ngo == 0)
        {
            if (rotateRightFlag)
            {
                pageIndex++;
                if (pageIndex % 2 == 1)
                {
                    selectChannel();
                    currentStateTemp = SELECT_CHANNEL_MODE;
                    Serial.println("SELECT_CHANNEL_MODE ma leaw ror");
                }
                else if (pageIndex % 2 == 0)
                {
                    onofftimer();
                    currentStateTemp = ON_OFF_TIMER_MODE;
                    Serial.println("ON_OFF_TIMER_MODE ma leaw ror");
                }
                resetflag();
            }
            else if (rotateLeftFlag)
            {
                pageIndex--;
                if (pageIndex % 2 == 1)
                {
                    selectChannel();
                    currentStateTemp = SELECT_CHANNEL_MODE;
                    Serial.println("SELECT_CHANNEL_MODE ma leaw rol");
                }
                else if (pageIndex % 2 == 0)
                {
                    onofftimer();
                    currentStateTemp = ON_OFF_TIMER_MODE;
                    Serial.println("ON_OFF_TIMER_MODE ma leaw rol");
                }
                resetflag();
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
    break;
    case SELECT_CHANNEL_MODE:
    {
        State currentStateTemp1 = ADJUST_INTENSITY__CH1_MODE;
        selectedChannel1();
        int ngo1 = 0;
        Serial.println("SELECT_CHANNEL_MODE ma leaw");
        while (ngo1 == 0)
        {
            if (rotateRightFlag)
            {
                // increment index
                pageIndex++;
                if (pageIndex % 5 == 0)
                {
                    Serial.println("SELECTING_CHANNEL_1 ma leaw ror");
                    selectingChannel1();
                    currentStateTemp1 = ADJUST_INTENSITY__CH1_MODE;
                }
                else if (pageIndex % 5 == 1)
                {
                    Serial.println("SELECTING_CHANNEL_2 ma leaw ror");
                    selectingChannel2();
                    currentStateTemp1 = ADJUST_INTENSITY__CH2_MODE;
                }
                else if (pageIndex % 5 == 2)
                {
                    Serial.println("SELECTING_CHANNEL_3 ma leaw ror");
                    selectingChannel3();
                    currentStateTemp1 = ADJUST_INTENSITY__CH3_MODE;
                }
                else if (pageIndex % 5 == 3)
                {
                    Serial.println("SELECTING_CHANNEL_4 ma leaw ror");
                    selectingChannel4();
                    currentStateTemp1 = ADJUST_INTENSITY__CH4_MODE;
                }
                else if (pageIndex % 5 == 4)
                {
                    Serial.println("SELECTING_CHANNEL_5 ma leaw ror");
                    selectingChannel5();
                    currentStateTemp1 = ADJUST_INTENSITY__CH5_MODE;
                }
                resetflag();
            }
            else if (rotateLeftFlag)
            {
                // decrement index
                pageIndex--;
                if (pageIndex % 5 == 0)
                {
                    Serial.println("SELECTING_CHANNEL_1 ma leaw rol");
                    selectingChannel1();
                    currentStateTemp1 = ADJUST_INTENSITY__CH1_MODE;
                }
                else if (pageIndex % 5 == 1)
                {
                    Serial.println("SELECTING_CHANNEL_2 ma leaw rol");
                    selectingChannel2();
                    currentStateTemp1 = ADJUST_INTENSITY__CH2_MODE;
                }
                else if (pageIndex % 5 == 2)
                {
                    Serial.println("SELECTING_CHANNEL_3 ma leaw rol");
                    selectingChannel3();
                    currentStateTemp1 = ADJUST_INTENSITY__CH3_MODE;
                }
                else if (pageIndex % 5 == 3)
                {
                    Serial.println("SELECTING_CHANNEL_4 ma leaw rol");
                    selectingChannel4();
                    currentStateTemp1 = ADJUST_INTENSITY__CH4_MODE;
                }
                else if (pageIndex % 5 == 4)
                {
                    Serial.println("SELECTING_CHANNEL_5 ma leaw rol");
                    selectingChannel5();
                    currentStateTemp1 = ADJUST_INTENSITY__CH5_MODE;
                }
                resetflag();
            }
            else if (buttonPressFlag)
            {
                ngo1 = true;
                currentState = currentStateTemp1;
                Serial.println(currentState);
                resetflag();
            }
        }
    }
    break;
    case ADJUST_INTENSITY__CH1_MODE:
    {
        //Serial.println("ADJUST_INTENSITY__CH1_MODE ma leaw");
        if(rotateLeftFlag || rotateRightFlag){
            intensity[0] = constrain(rotaryEncoder.readEncoder(), 0, 100);
            Serial.print("Intensity[0]: ");
            Serial.println(intensity[0]);
            resetflag();
        }
        else if(buttonPressFlag){
            EEPROM.write(EEPROM_ADDRESS + 0, intensity[0]);
            EEPROM.commit();
            Serial.println("Intensity[0] stored in EEPROM");
            buttonPressFlag = false;
        }
        resetflag();
    }
    break;
    case ADJUST_INTENSITY__CH2_MODE:
    {
        Serial.println("ADJUST_INTENSITY__CH2_MODE ma leaw");
        resetflag();
    }
    break;
    case ADJUST_INTENSITY__CH3_MODE:
    {
        Serial.println("ADJUST_INTENSITY__CH3_MODE ma leaw");
        resetflag();
    }
    break;
    case ADJUST_INTENSITY__CH4_MODE:
    {
        Serial.println("ADJUST_INTENSITY__CH4_MODE ma leaw");
        resetflag();
    }
    break;
    case ADJUST_INTENSITY__CH5_MODE:
    {
        Serial.println("ADJUST_INTENSITY__CH5_MODE ma leaw");
        resetflag();
    }
    break;

    case ON_OFF_TIMER_MODE:
    {
        Serial.println("ON_OFF_TIMER_MODE ma leaw");
        currentState = SELECT_CHANNEL_MODE;
        resetflag();
    }
    break;
    }
    delay(100);
}
//for debugging purpose
// void touchTask(void *pvParameters)
// {
//     for (;;)
//     {
//         int touch1 = touchRead(TOUCH_PIN_1); // Read GPIO 32
//         if (touch1 < TOUCH_THRESHOLD)
//         {
//             rotateRightFlag = true;
//             Serial.println("TOUCH_PIN_1 touched rotating right");
//         }
//         else
//         {
//             rotateRightFlag = false;
//         }
//         vTaskDelay(pdMS_TO_TICKS(500)); // Delay for 500ms

//         int touch2 = touchRead(TOUCH_PIN_2); // Read GPIO 33
//         if (touch2 < TOUCH_THRESHOLD)
//         {
//             rotateLeftFlag = true;
//             Serial.println("TOUCH_PIN_2 touched rotating left");
//         }
//         else
//         {
//             rotateLeftFlag = false;
//         }
//         vTaskDelay(pdMS_TO_TICKS(500)); // Delay for 500ms

//         int touch3 = touchRead(TOUCH_PIN_3); // Read GPIO 12
//         if (touch3 < TOUCH_THRESHOLD)
//         {
//             buttonPressFlag = true;
//             Serial.println("TOUCH_PIN_3 touched button pressed");
//         }
//         else
//         {
//             buttonPressFlag = false;
//         }
//         vTaskDelay(pdMS_TO_TICKS(500)); // Delay for 500ms

//         int touch4 = touchRead(TOUCH_PIN_4); // Read GPIO 14
//         if (touch4 < TOUCH_THRESHOLD)
//         {
//             buttonHeldFlag = true;
//             Serial.println("TOUCH_PIN_4 touched button held");
//         }
//         else
//         {
//             buttonHeldFlag = false;
//         }
//         vTaskDelay(pdMS_TO_TICKS(500)); // Delay for 500ms
//     }
// }

void lcdHomepage()
{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("HOMEPAGE");
    lcd.setCursor(0, 1);
    lcd.print("Time: ");
    lcd.print("13:00");
}

// Layer 2
void selectChannel()
{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("<SELECT CHANNEL MODE>");
    // lcd.print(channel);
}

// Layer 3
void onofftimer()
{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("<ON/OFF TIMER>");
}

// Selecting channel
void selectingChannel1()
{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("<Channel 1>");
    lcd.setCursor(0, 1);
    lcd.print("Intensity: ");
    lcd.print(intensity[1]);
}
void selectingChannel2()
{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("<Channel 2>");
    lcd.setCursor(0, 1);
    lcd.print("Intensity: ");
    lcd.print(intensity[2]);
}
void selectingChannel3()
{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("<Channel 3>");
    lcd.setCursor(0, 1);
    lcd.print("Intensity: ");
    lcd.print(intensity[3]);
}
void selectingChannel4()
{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("<Channel 4>");
    lcd.setCursor(0, 1);
    lcd.print("Intensity: ");
    lcd.print(intensity[4]);
}
void selectingChannel5()
{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("<Channel 5>");
    lcd.setCursor(0, 1);
    lcd.print("Intensity: ");
    lcd.print(intensity[5]);
}
// Select channel
void selectedChannel1()
{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Channel 1");
    lcd.setCursor(0, 1);
    lcd.print("Intensity:");
    lcd.print(intensity[1]);
}
void selectedChannel2()
{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Channel 2");
    lcd.setCursor(0, 1);
    lcd.print("Intensity: ");
    lcd.print(intensity[2]);
}
void selectedChannel3()
{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Channel 3");
    lcd.setCursor(0, 1);
    lcd.print("Intensity: ");
    lcd.print(intensity[3]);
}
void selectedChannel4()
{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Channel 4");
    lcd.setCursor(0, 1);
    lcd.print("Intensity: ");
    lcd.print(intensity[4]);
}
void selectedChannel5()
{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Channel 5");
    lcd.setCursor(0, 1);
    lcd.print("Intensity: ");
    lcd.print(intensity[5]);
}
void resetflag(){
    rotateLeftFlag = false;
    rotateRightFlag = false;
    buttonPressFlag = false;
    buttonHeldFlag = false;
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