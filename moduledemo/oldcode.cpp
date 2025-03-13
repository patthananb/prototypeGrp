#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Encoder.h>
#include <WiFi.h>

/* to do lists

button held must have 1 second and 2 seconds

action
-press                  select
-long press 1 seconds   edit
-long press 2 seconds   return home

anaglog write to ledc
*/

// LCD setup
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Rotary encoder setup
#define ROTARY_CLK 11
#define ROTARY_DT 12
#define ROTARY_SW 13
Encoder myEnc(ROTARY_CLK, ROTARY_DT);

// LED pins
#define LED1 8
#define LED2 9
#define LED3 10
#define LED4 6
#define LED5 7

void displayHomePage();
void updateLEDs();
void channel1();
void channel2();
void channel3();
void channel4();
void channel5();
void rotaryEncoderTask(void *pvParameters);
void lcdTask(void *pvParameters);
void wifiTask(void *pvParameters);

int channel = 1; // Current channel
bool selectingChannel = false; // Channel selection mode flag
bool editingChannel = false; // Channel editing mode flag
long lastPressTime = 0;
bool buttonHeld = false;
long lastActivityTime = 0; // Track last activity time
int pwmValues[5] = {255, 255, 255, 255, 255}; // PWM values for each channel

const char* ssid = "your_SSID";
const char* password = "your_PASSWORD";

void setup() {
    // Initialize Serial
    Serial.begin(115200);

    // Initialize WiFi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }
    Serial.println("Connected to WiFi");

    // Initialize GPIO
    pinMode(ROTARY_SW, INPUT_PULLUP);
    pinMode(LED1, OUTPUT);
    pinMode(LED2, OUTPUT);
    pinMode(LED3, OUTPUT);
    pinMode(LED4, OUTPUT);
    pinMode(LED5, OUTPUT);

    // Initialize LCD
    lcd.init();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("WELCOME");
    lcd.setCursor(0, 1);
    lcd.print("Initializing...");
    delay(2000);
    displayHomePage();
    updateLEDs();

    // Create FreeRTOS tasks
    xTaskCreate(rotaryEncoderTask, "Rotary Encoder Task", 2048, NULL, 1, NULL);
    xTaskCreate(lcdTask, "LCD Task", 2048, NULL, 1, NULL);
    xTaskCreate(wifiTask, "WiFi Task", 2048, NULL, 1, NULL);
}

void loop() {
    // Empty loop as tasks are managed by FreeRTOS
}

void rotaryEncoderTask(void *pvParameters) {
    static long lastPosition = 0;
    while (1) {
        long newPosition = myEnc.read() / 2;
        // Check for timeout in selection mode
        if ((selectingChannel || editingChannel) && (millis() - lastActivityTime > 3000)) {
            selectingChannel = false;
            editingChannel = false;
            displayHomePage();
            updateLEDs();
        }

        if (digitalRead(ROTARY_SW) == LOW) {
            if (!buttonHeld) {
                lastPressTime = millis();
                buttonHeld = true;
                lastActivityTime = millis(); // Reset activity timer on button press
            }
        } else {
            if (buttonHeld) {
                if (editingChannel) {
                    // Exit editing mode with a short press
                    editingChannel = false;
                    updateLEDs();
                    displayHomePage();
                } else if (selectingChannel) {
                    // Enter editing mode with a short press
                    selectingChannel = false;
                    editingChannel = true;
                    lcd.clear();
                    lcd.setCursor(0, 0);
                    lcd.print("EDIT CHANNEL");
                    lcd.setCursor(0, 1);
                    lcd.print("Intensity: ");
                    lcd.print(pwmValues[channel - 1]);
                } else if (millis() - lastPressTime > 1000) { // Detect long press and release
                    selectingChannel = true;
                    lcd.clear();
                    lcd.setCursor(0, 0);
                    lcd.print("SELECT CHANNEL");
                    lcd.setCursor(0, 1);
                    lcd.print("Channel: ");
                    lcd.print(channel);
                    lastActivityTime = millis(); // Reset activity timer when entering selection mode
                }
                
                buttonHeld = false;
            }
        }

        if (selectingChannel && newPosition != lastPosition) {
            if (newPosition > lastPosition) {
                channel++;
            } else {
                channel--;
            }
            //cycling between channels
            if (channel > 5) channel = 1;
            if (channel < 1) channel = 5;

            lastActivityTime = millis(); // Reset activity timer when encoder is turned
            lastPosition = newPosition;
            lcd.setCursor(9, 1);
            lcd.print(channel);
        }

        if (editingChannel && newPosition != lastPosition) {
            if (newPosition > lastPosition) {
                pwmValues[channel - 1] += 5;
            } else {
                pwmValues[channel - 1] -= 5;
            }
            // Constrain PWM values between 0 and 255
            if (pwmValues[channel - 1] > 255) pwmValues[channel - 1] = 255;
            if (pwmValues[channel - 1] < 0) pwmValues[channel - 1] = 0;

            lastActivityTime = millis(); // Reset activity timer when encoder is turned
            lastPosition = newPosition;
            lcd.setCursor(11, 1);
            lcd.print(pwmValues[channel - 1]);
            updateLEDs();
        }

        vTaskDelay(pdMS_TO_TICKS(10)); // Delay to prevent task from hogging CPU
    }
}

void lcdTask(void *pvParameters) {
    while (1) {
        // Update LCD display if needed
        vTaskDelay(pdMS_TO_TICKS(1000)); // Adjust delay as needed
    }
}

void wifiTask(void *pvParameters) {
    while (1) {
        // Maintain WiFi connection
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("Reconnecting to WiFi...");
            WiFi.reconnect();
        }
        vTaskDelay(pdMS_TO_TICKS(60000)); // Check WiFi connection every 1 minute
    }
}

void displayHomePage() {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("HOMEPAGE");
    lcd.setCursor(0, 1);
    lcd.print("Channel: ");
    lcd.print(channel);
}

void updateLEDs() {
    switch (channel) {
        case 1: channel1(); break;
        case 2: channel2(); break;
        case 3: channel3(); break;
        case 4: channel4(); break;
        case 5: channel5(); break;
    }
}

void channel1() {
    analogWrite(LED1, pwmValues[0]);
    analogWrite(LED2, 0);
    analogWrite(LED3, 0);
    analogWrite(LED4, 0);
    analogWrite(LED5, 0);
}

void channel2() {
    analogWrite(LED1, 0);
    analogWrite(LED2, pwmValues[1]);
    analogWrite(LED3, 0);
    analogWrite(LED4, 0);
    analogWrite(LED5, 0);
}

void channel3() {
    analogWrite(LED1, 0);
    analogWrite(LED2, 0);
    analogWrite(LED3, pwmValues[2]);
    analogWrite(LED4, 0);
    analogWrite(LED5, 0);
}

void channel4() {
    analogWrite(LED1, 0);
    analogWrite(LED2, 0);
    analogWrite(LED3, 0);
    analogWrite(LED4, pwmValues[3]);
    analogWrite(LED5, 0);
}

void channel5() {
    analogWrite(LED1, 0);
    analogWrite(LED2, 0);
    analogWrite(LED3, 0);
    analogWrite(LED4, 0);
    analogWrite(LED5, pwmValues[4]);
}
/* 
fix analog write functions
to ledc
*/