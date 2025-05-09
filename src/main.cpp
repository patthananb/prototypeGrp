#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <LiquidCrystal_I2C.h>
#include <AiEsp32RotaryEncoder.h>
#include <EEPROM.h>

#define ROTARY_ENCODER_A_PIN 34
#define ROTARY_ENCODER_B_PIN 35
#define ROTARY_ENCODER_BUTTON_PIN 32
#define ROTARY_ENCODER_STEPS 1

#define PWM_FAN 14 // GPIO 14 for Fan PWM

// Track state
static unsigned long lastKnobChange = 0;
static long lastEncoderValue = 0;
static bool lastButtonState = true;
static unsigned long buttonDownTime = 0;
static unsigned long lastLongPressTime = 0;
static unsigned long lastActivityTime = 0;

#define LONG_PRESS_TIME 1000
static const unsigned long DEBOUNCE_DELAY_KNOB = 50;
static const unsigned long INACTIVITY_TIMEOUT = 10000;

//  EEPORM address
const int address_Intensity0 = 1; // address EEPROM
const int address_Intensity1 = 2;
const int address_Intensity2 = 3;
const int address_Intensity3 = 4;
const int address_Intensity4 = 5;
const int address_masterVolume = 6;

const int pwmLed2 = 27;      // GPIO27 for PWM
const int pwmLed5 = 12;      // GPIO12 for PWM
const int pwmLed4 = 26;      // GPIO26 for PWM
const int pwmLed3 = 25;      // GPIO25 for PWM
const int pwmLed1 = 33;      // GPIO33 for PWM
const int pwmFreq = 20000;   // Frequency 20 kHz
const int pwmResolution = 8; // 8-bit resolution (0-255)

// Behavior flags
volatile bool rotateLeftFlag = false;
volatile bool rotateRightFlag = false;
volatile bool buttonPressFlag = false;
volatile bool buttonHeldFlag = false;
volatile unsigned long buttonPressStartTime = 0; // เวลาที่เริ่มกดปุ่ม
int pageIndex = 0;
int Intensity[5] = {};
// [ch1, ch2, ch3, ch4, ch5, masterVolume, mode]

// lcd functions protptype
void lcdHomepage();
void selectChannel();
void onofftimer();

void selectedChannel1();
void selectedChannel2();
void selectedChannel3();
void selectedChannel4();
void selectedChannel5();
void selectedMasterVolume();
void resetflag();
void EEPROM_SET();
void PWM_Setting();
void writeArrayToEEPROM(int arr[], int size);
void readArrayFromEEPROM(int arr[], int size);
void writeLED();

enum State
{
    HOME,
    MODE_SELECT,
    SELECT_CHANNEL_MODE,
    ON_OFF_TIMER_MODE,
    ADJUST_INTENSITY_MASTER_VOLUME,
    ADJUST_INTENSITY__CH1_MODE,
    ADJUST_INTENSITY__CH2_MODE,
    ADJUST_INTENSITY__CH3_MODE,
    ADJUST_INTENSITY__CH4_MODE,
    ADJUST_INTENSITY__CH5_MODE,
};

State currentState = HOME;

LiquidCrystal_I2C lcd(0x27, 16, 2);



AiEsp32RotaryEncoder rotaryEncoder = AiEsp32RotaryEncoder(ROTARY_ENCODER_A_PIN, ROTARY_ENCODER_B_PIN, ROTARY_ENCODER_STEPS);

// ฟังก์ชัน callback ทำงานเมื่อ Encoder หมุน
void IRAM_ATTR readEncoderISR()
{
    rotaryEncoder.readEncoder_ISR();

    long currentPosition = rotaryEncoder.readEncoder(); // อ่านค่าปัจจุบัน
    static long lastPosition = 0;                       // เก็บค่าตำแหน่งก่อนหน้า
    unsigned long currentTime = millis();
    if (currentTime - lastKnobChange > DEBOUNCE_DELAY_KNOB)
    {
        lastKnobChange = currentTime;
        if (currentPosition > lastPosition)
        {
            rotateRightFlag = true; // หมุนตามเข็ม
            // rotateLeftFlag = false;
            Serial.println("Rotated Right");
        }
        else if (currentPosition < lastPosition)
        {
            rotateLeftFlag = true; // หมุนทวนเข็ม
            // rotateRightFlag = false;
            Serial.println("Rotated Left");
        }
        lastPosition = currentPosition; // อัปเดตตำแหน่งล่าสุด
    }
}
void IRAM_ATTR buttonISR()
{
    static unsigned long lastInterruptTime = 0;
    unsigned long interruptTime = millis();

    if (interruptTime - lastInterruptTime > 200)
    { // กันสัญญาณดีด (debounce)
        buttonPressFlag = true;
        buttonPressStartTime = interruptTime; // บันทึกเวลาที่เริ่มกด
        Serial.println("ปุ่มถูกกด!");
    }

    lastInterruptTime = interruptTime;
}

void setup()
{
    Serial.begin(115200);
    pinMode(ROTARY_ENCODER_BUTTON_PIN, INPUT_PULLUP);
    pinMode(PWM_FAN, OUTPUT);
    PWM_Setting();
    resetflag();
    rotaryEncoder.begin();
    rotaryEncoder.setup([]{ readEncoderISR(); });
    rotaryEncoder.setBoundaries(-10000, 100000, true);
    rotaryEncoder.setAcceleration(0);
    lastEncoderValue = rotaryEncoder.readEncoder();
    lastButtonState = true;
    attachInterrupt(digitalPinToInterrupt(ROTARY_ENCODER_BUTTON_PIN), buttonISR, FALLING);
    EEPROM_SET();
    lcd.init();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("WELCOME");
    lcd.setCursor(0, 1);
    lcd.print("Initializing...");
    delay(500);
    currentState = HOME;
}

void loop()
{
    // if (Serial.available())
    // {
    //     String command = Serial.readStringUntil('\n');
    //     if (command == "printinten")
    //     {
    //         Serial.println("EEPROM Values:");
    //         for (int i = 0; i < 4; i++)
    //         {
    //             Serial.print("Intensity[");
    //             Serial.print(i);
    //             Serial.print("]: ");
    //             Serial.println(Intensity[i]);
    //             Serial.println(EEPROM.read(address_Intensity0 + i));
    //         }
    //     }
    // }

    /*
        we need to multiply with factor MasterVolume to all index of intensity
        trig fan pin to always on
    */

    // //master volume factor
    // for (int i = 0; i < 5; i++)
    // {
    //     Intensity[i] = Intensity[i] * Intensity[5] / 100;
    // }


    //writeLED();

    digitalWrite(PWM_FAN, HIGH); // Turn the fan on

    switch (currentState)
    {
    case HOME:
    {
        Serial.println("HOME malaew");
        lcd.clear();
        int ngo = 0;
        // display homepage
        while (ngo == 0)
        {
            lcdHomepage();
            if (buttonPressFlag == true)
            {
                // Serial.println("button held ma if laew jing jing na");
                currentState = MODE_SELECT;
                ngo = 1;
                resetflag();
            }
        }
    }
    break;
    case MODE_SELECT:
    {
        State currentStateTemp = SELECT_CHANNEL_MODE;
        unsigned long previousTime = millis(); // จับเวลาครั้งแรก
        int previousValue = -1;
        int ngo = 0;
        selectChannel();
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
                    previousTime = millis();
                    Serial.println("SELECT_CHANNEL_MODE ma leaw ror");
                }
                else if (pageIndex % 2 == 0)
                {
                    onofftimer();
                    currentStateTemp = ON_OFF_TIMER_MODE;
                    previousTime = millis();
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
                    previousTime = millis();
                    Serial.println("SELECT_CHANNEL_MODE ma leaw rol");
                }
                else if (pageIndex % 2 == 0)
                {
                    onofftimer();
                    currentStateTemp = ON_OFF_TIMER_MODE;
                    previousTime = millis();
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
            else if (millis() - previousTime > 3000)
            {
                ngo = 1;
                currentState = HOME;
                pageIndex = 0; // reset page index
                resetflag();
            }
        }
    }
    break;
    case SELECT_CHANNEL_MODE:
    {
        State currentStateTemp1 = ADJUST_INTENSITY__CH1_MODE;
        unsigned long previousTime = millis(); // จับเวลาครั้งแรก
        int previousValue = -1;
        selectedChannel1();
        int ngo1 = 0;
        Serial.println("SELECT_CHANNEL_MODE ma leaw");
        while (ngo1 == 0)
        {
            if (rotateRightFlag)
            {
                // increment index
                pageIndex++;
                if (pageIndex % 6 == 0)
                {
                    Serial.println("SELECTING_CHANNEL_1 ma leaw ror");
                    selectedChannel1();
                    currentStateTemp1 = ADJUST_INTENSITY__CH1_MODE;
                    previousTime = millis();
                }
                else if (pageIndex % 6 == 1 | pageIndex % 6 == -1)
                {
                    Serial.println("SELECTING_CHANNEL_2 ma leaw ror");
                    selectedChannel2();
                    currentStateTemp1 = ADJUST_INTENSITY__CH2_MODE;
                    previousTime = millis();
                }
                else if (pageIndex % 6 == 2 | pageIndex % 6 == -2)
                {
                    Serial.println("SELECTING_CHANNEL_3 ma leaw ror");
                    selectedChannel3();
                    currentStateTemp1 = ADJUST_INTENSITY__CH3_MODE;
                    previousTime = millis();
                }
                else if (pageIndex % 6 == 3 | pageIndex % 6 == -3)
                {
                    Serial.println("SELECTING_CHANNEL_4 ma leaw ror");
                    selectedChannel4();
                    currentStateTemp1 = ADJUST_INTENSITY__CH4_MODE;
                    previousTime = millis();
                }
                else if (pageIndex % 6 == 4 | pageIndex % 6 == -4)
                {
                    Serial.println("SELECTING_CHANNEL_5 ma leaw ror");
                    selectedChannel5();
                    currentStateTemp1 = ADJUST_INTENSITY__CH5_MODE;
                    previousTime = millis();
                }
                else if (pageIndex % 6 == 5 | pageIndex % 6 == -5)
                {
                    Serial.println("SELECTING_CHANNEL_masterVolume ma leaw ror");
                    selectedMasterVolume();
                    currentStateTemp1 = ADJUST_INTENSITY_MASTER_VOLUME;
                    previousTime = millis();
                }
                resetflag();
            }
            else if (rotateLeftFlag)
            {
                // decrement index
                pageIndex--;
                if (pageIndex % 6 == 0)
                {
                    Serial.println("SELECTING_CHANNEL_1 ma leaw rol");
                    selectedChannel1();
                    currentStateTemp1 = ADJUST_INTENSITY__CH1_MODE;
                    previousTime = millis();
                }
                else if (pageIndex % 6 == 1 | pageIndex % 6 == -1)
                {
                    Serial.println("SELECTING_CHANNEL_2 ma leaw rol");
                    selectedChannel2();
                    currentStateTemp1 = ADJUST_INTENSITY__CH2_MODE;
                    previousTime = millis();
                }
                else if (pageIndex % 6 == 2 | pageIndex % 6 == -2)
                {
                    Serial.println("SELECTING_CHANNEL_3 ma leaw rol");
                    selectedChannel3();
                    currentStateTemp1 = ADJUST_INTENSITY__CH3_MODE;
                    previousTime = millis();
                }
                else if (pageIndex % 6 == 3 | pageIndex % 6 == -3)
                {
                    Serial.println("SELECTING_CHANNEL_4 ma leaw rol");
                    selectedChannel4();
                    currentStateTemp1 = ADJUST_INTENSITY__CH4_MODE;
                    previousTime = millis();
                }
                else if (pageIndex % 6 == 4 | pageIndex % 6 == -4)
                {
                    Serial.println("SELECTING_CHANNEL_5 ma leaw rol");
                    selectedChannel5();
                    currentStateTemp1 = ADJUST_INTENSITY__CH5_MODE;
                    previousTime = millis();
                }
                else if (pageIndex % 6 == 5 | pageIndex % 6 == -5)
                {
                    Serial.println("SELECTING_CHANNEL_MASTER_VOLUME ma leaw rol");
                    selectedMasterVolume();
                    currentStateTemp1 = ADJUST_INTENSITY_MASTER_VOLUME;
                    previousTime = millis();
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
            else if (millis() - previousTime > 3000)
            {
                ngo1 = 1;
                currentState = HOME;
                pageIndex = 0; // reset page index
                resetflag();
            }
        }
    }
    break;
    case ADJUST_INTENSITY__CH1_MODE:
    {
        Serial.println("ADJUST_INTENSITY__CH1_MODE ma leaw");
        // unsigned long previousTime = millis();  // จับเวลาครั้งแรก
        int ngo = 0;
        int PWM_Value;
        PWM_Value = Intensity[0];
        lcd.clear();
        while (ngo == 0)
        {
            PWM_Value = constrain(PWM_Value, 0, 100);
            // int pre_pwm = PWM_Value;
            // Serial.println(PWM_Value);
            if (rotateRightFlag == true)
            {
                PWM_Value++;
                Serial.println(PWM_Value);
                resetflag();
                lcd.setCursor(12, 0);
                lcd.print("   "); // ลบค่าก่อนหน้า

                // delay(10);
            }
            else if (rotateLeftFlag)
            {
                PWM_Value--;
                Serial.println(PWM_Value);
                resetflag();
                lcd.setCursor(12, 0);
                lcd.print("   "); // ลบค่าก่อนหน้า
                                  // delay(10);
            }
            else if (buttonPressFlag)
            {
                Intensity[0] = PWM_Value;
                EEPROM.write(address_Intensity0, Intensity[0]);
                EEPROM.commit();

                currentState = SELECT_CHANNEL_MODE;
                ngo = 1;
            }
            pwmAdjust(0, PWM_Value);
            Lcd_adjustPWM(PWM_Value);
        }

        resetflag();
    }
    break;
    case ADJUST_INTENSITY__CH2_MODE:
    {
        Serial.println("ADJUST_INTENSITY__CH2_MODE ma leaw");
        // unsigned long previousTime = millis();  // จับเวลาครั้งแรก
        int ngo = 0;
        int PWM_Value;
        PWM_Value = Intensity[1];
        lcd.clear();
        while (ngo == 0)
        {
            PWM_Value = constrain(PWM_Value, 0, 100);
            // int pre_pwm = PWM_Value;
            // Serial.println(PWM_Value);
            if (rotateRightFlag == true)
            {
                PWM_Value++;
                Serial.println(PWM_Value);
                resetflag();
                lcd.setCursor(12, 0);
                lcd.print("   "); // ลบค่าก่อนหน้า

                // delay(10);
            }
            else if (rotateLeftFlag)
            {
                PWM_Value--;
                Serial.println(PWM_Value);
                resetflag();
                lcd.setCursor(12, 0);
                lcd.print("   "); // ลบค่าก่อนหน้า
                // delay(10);
            }
            else if (buttonPressFlag)
            {
                Intensity[1] = PWM_Value;
                EEPROM.write(address_Intensity1, Intensity[1]);
                EEPROM.commit();

                currentState = SELECT_CHANNEL_MODE;
                ngo = 1;
            }
            pwmAdjust(1, PWM_Value);
            Lcd_adjustPWM(PWM_Value);
        }

        resetflag();
    }
    break;
    case ADJUST_INTENSITY__CH3_MODE:
    {
        Serial.println("ADJUST_INTENSITY__CH1_MODE ma leaw");
        // unsigned long previousTime = millis();  // จับเวลาครั้งแรก
        int ngo = 0;
        int PWM_Value;
        PWM_Value = Intensity[2];
        lcd.clear();
        while (ngo == 0)
        {
            PWM_Value = constrain(PWM_Value, 0, 100);
            // int pre_pwm = PWM_Value;
            // Serial.println(PWM_Value);
            if (rotateRightFlag == true)
            {
                PWM_Value++;
                Serial.println(PWM_Value);
                resetflag();
                lcd.setCursor(12, 0);
                lcd.print("   "); // ลบค่าก่อนหน้า

                // delay(10);
            }
            else if (rotateLeftFlag)
            {
                PWM_Value--;
                Serial.println(PWM_Value);
                resetflag();
                lcd.setCursor(12, 0);
                lcd.print("   "); // ลบค่าก่อนหน้า
                                  // delay(10);
            }
            else if (buttonPressFlag)
            {
                Intensity[2] = PWM_Value;
                EEPROM.write(address_Intensity2, Intensity[2]);
                EEPROM.commit();

                currentState = SELECT_CHANNEL_MODE;
                ngo = 1;
            }
            pwmAdjust(2, PWM_Value);
            Lcd_adjustPWM(PWM_Value);
        }

        resetflag();
    }
    break;
    case ADJUST_INTENSITY__CH4_MODE:
    {
        Serial.println("ADJUST_INTENSITY__CH4_MODE ma leaw");
        // unsigned long previousTime = millis();  // จับเวลาครั้งแรก
        int ngo = 0;
        int PWM_Value;
        PWM_Value = Intensity[3];
        lcd.clear();
        while (ngo == 0)
        {
            PWM_Value = constrain(PWM_Value, 0, 100);
            // int pre_pwm = PWM_Value;
            // Serial.println(PWM_Value);
            if (rotateRightFlag == true)
            {
                PWM_Value++;
                Serial.println(PWM_Value);
                resetflag();
                lcd.setCursor(12, 0);
                lcd.print("   "); // ลบค่าก่อนหน้า

                // delay(10);
            }
            else if (rotateLeftFlag)
            {
                PWM_Value--;
                Serial.println(PWM_Value);
                resetflag();
                lcd.setCursor(12, 0);
                lcd.print("   "); // ลบค่าก่อนหน้า
                // delay(10);
            }
            else if (buttonPressFlag)
            {
                Intensity[3] = PWM_Value;
                EEPROM.write(address_Intensity3, Intensity[3]);
                EEPROM.commit();

                currentState = SELECT_CHANNEL_MODE;
                ngo = 1;
            }
            pwmAdjust(3, PWM_Value);
            Lcd_adjustPWM(PWM_Value);
        }

        resetflag();
    }
    break;
    case ADJUST_INTENSITY__CH5_MODE:
    {
        Serial.println("ADJUST_INTENSITY__CH5_MODE ma leaw");
        // unsigned long previousTime = millis();  // จับเวลาครั้งแรก
        int ngo = 0;
        int PWM_Value;
        PWM_Value = Intensity[4];
        lcd.clear();
        while (ngo == 0)
        {
            PWM_Value = constrain(PWM_Value, 0, 100);
            // int pre_pwm = PWM_Value;
            // Serial.println(PWM_Value);
            if (rotateRightFlag == true)
            {
                PWM_Value++;
                Serial.println(PWM_Value);
                resetflag();
                lcd.setCursor(12, 0);
                lcd.print("   "); // ลบค่าก่อนหน้า

                // delay(10);
            }
            else if (rotateLeftFlag)
            {
                PWM_Value--;
                Serial.println(PWM_Value);
                resetflag();
                lcd.setCursor(12, 0);
                lcd.print("   "); // ลบค่าก่อนหน้า
                                  // delay(10);
            }
            else if (buttonPressFlag)
            {
                Intensity[4] = PWM_Value;
                EEPROM.write(address_Intensity4, Intensity[4]);
                EEPROM.commit();

                currentState = SELECT_CHANNEL_MODE;
                ngo = 1;
            }
            pwmAdjust(4, PWM_Value);
            Lcd_adjustPWM(PWM_Value);
        }

        resetflag();
    }
    break;

    case ADJUST_INTENSITY_MASTER_VOLUME:
    {
        Serial.println("ADJUST_INTENSITY_MASTER_VOLUME ma leaw");
        int ngo = 0;
        int masterVolume;
        masterVolume = Intensity[4];
        lcd.clear();
        while (ngo == 0)
        {
            masterVolume = constrain(masterVolume, 0, 100);
            if (rotateRightFlag == true)
            {
                masterVolume++;
                Serial.println(masterVolume);
                resetflag();
                lcd.setCursor(12, 0);
                lcd.print("   "); // ลบค่าก่อนหน้า

                // delay(10);
            }
            else if (rotateLeftFlag)
            {
                masterVolume--;
                Serial.println(masterVolume);
                resetflag();
                lcd.setCursor(12, 0);
                lcd.print("   "); // ลบค่าก่อนหน้า
                                  // delay(10);
            }
            else if (buttonPressFlag)
            {
                Intensity[5] = masterVolume;
                EEPROM.write(address_masterVolume, Intensity[5]);
                EEPROM.commit();

                currentState = SELECT_CHANNEL_MODE;
                ngo = 1;
            }
            Lcd_adjustPWM(masterVolume);
        }

        resetflag();
    }

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

void lcdHomepage()
{
    // lcd.clear();
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
    lcd.print("SETTING");
    lcd.setCursor(0, 1);
    lcd.print(">> BRIGHTNESS");
    // lcd.print(channel);
}

// Layer 3
void onofftimer()
{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("SETTING");
    lcd.setCursor(0, 1);
    lcd.print(">> TIMER");
}

// Select channel
void selectedChannel1()
{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("SELECT");
    lcd.setCursor(0, 1);
    lcd.print("Channel 1");
}
void selectedChannel2()
{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("SELECT");
    lcd.setCursor(0, 1);
    lcd.print("Channel 2");
}
void selectedChannel3()
{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("SELECT");
    lcd.setCursor(0, 1);
    lcd.print("Channel 3");
}
void selectedChannel4()
{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("SELECT");
    lcd.setCursor(0, 1);
    lcd.print("Channel 4");
}
void selectedChannel5()
{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("SELECT");
    lcd.setCursor(0, 1);
    lcd.print("Channel 5");
}
void selectedMasterVolume(){
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("SELECT");
    lcd.setCursor(0, 1);
    lcd.print("Master Volume");
}
void resetflag()
{
    rotateLeftFlag = false;
    rotateRightFlag = false;
    buttonPressFlag = false;
    buttonHeldFlag = false;
}
void writeArrayToEEPROM(int arr[], int size)
{
    for (int i = 0; i < size; i++)
    {
        EEPROM.write(address_Intensity0 + i, arr[i]);
    }
    EEPROM.commit();
    Serial.println("Array stored in EEPROM");
}

void readArrayFromEEPROM(int arr[], int size)
{
    for (int i = 0; i < size; i++)
    {
        arr[i] = EEPROM.read(address_Intensity0 + i);
    }
    Serial.println("Array read from EEPROM");
    for (int i = 0; i < size; i++)
    {
        Serial.print(arr[i]);
        Serial.print(" ");
    }
    Serial.println();
}
void EEPROM_SET()
{
    EEPROM.begin(20); // define size EEPROM
    Intensity[0] = EEPROM.read(address_Intensity0);
    Intensity[1] = EEPROM.read(address_Intensity1);
    Intensity[2] = EEPROM.read(address_Intensity2);
    Intensity[3] = EEPROM.read(address_Intensity3);
    Intensity[4] = EEPROM.read(address_Intensity4);
}
void Lcd_adjustPWM(int value)
{

    lcd.setCursor(1, 0); // กำหนดตำแหน่งสำหรับแสดงค่า
    lcd.print("Intensity: ");
    lcd.setCursor(12, 0); // กำหนดตำแหน่งสำหรับแสดงค่า
    // lcd.print("   "); // ลบค่าก่อนหน้า
    lcd.setCursor(12, 0);
    lcd.print(value);
    // แสดงแถบบาร์ในแถวที่ 2
    int barLength = map(value, 0, 100, 0, 16); // แปลงค่าเป็นความยาว 0-16
    lcd.setCursor(0, 1);
    for (int i = 0; i < 16; i++)
    {
        if (i < barLength)
        {
            lcd.write(byte(255)); // แสดงบล็อกเต็ม
        }
        else
        {
            lcd.print(" "); // เคลียร์พื้นที่ที่เหลือ
        }
    }
}

void PWM_Setting()
{
    ledcAttach(pwmLed1, pwmFreq, pwmResolution); // Set PWM on pin D5
    ledcAttach(pwmLed2, pwmFreq, pwmResolution); // Set PWM on pin D5
    ledcAttach(pwmLed3, pwmFreq, pwmResolution); // Set PWM on pin D5
    ledcAttach(pwmLed4, pwmFreq, pwmResolution); // Set PWM on pin D5
    ledcAttach(pwmLed5, pwmFreq, pwmResolution); // Set PWM on pin D5
}

//this functions is for demo intensity before saving
void pwmAdjust(int channel, int value)
{
    switch (channel)
    {
    case 0:
    {
        ledcWrite(pwmLed1, map(value, 0, 100, 0, 190));
    }
    break;
    case 1:
    {
        ledcWrite(pwmLed2, map(value, 0, 100, 0, 190));
    }
    break;
    case 2:
    {
        ledcWrite(pwmLed3, map(value, 0, 100, 0, 190));
    }
    break;
    case 3:
    {
        ledcWrite(pwmLed4, map(value, 0, 100, 0, 190));
    }
    break;
    case 4:
    {
        ledcWrite(pwmLed5, map(value, 0, 100, 0, 190));
    }
    break;
    }
}

void writeLED(){
    ledcWrite(pwmLed1, map(Intensity[0], 0, 100, 0, 255));
    ledcWrite(pwmLed2, map(Intensity[1], 0, 100, 0, 255));
    ledcWrite(pwmLed3, map(Intensity[2], 0, 100, 0, 255));
    ledcWrite(pwmLed4, map(Intensity[3], 0, 100, 0, 255));
    ledcWrite(pwmLed5, map(Intensity[4], 0, 100, 0, 255));
}

void off(){
    ledcWrite(pwmLed1, 0);
    ledcWrite(pwmLed2, 0);
    ledcWrite(pwmLed3, 0);
    ledcWrite(pwmLed4, 0);
    ledcWrite(pwmLed5, 0);
}
