#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <LiquidCrystal_I2C.h>

#define TOUCH_PIN_1 32  // GPIO 32
#define TOUCH_PIN_2 33  // GPIO 33
#define TOUCH_PIN_3 12  // GPIO 12
#define TOUCH_PIN_4 14  // GPIO 14

// Set a threshold based on your board's sensitivity
#define TOUCH_THRESHOLD 30  

// Behavior flags
bool rotateRightFlag = false;
bool rotateLeftFlag = false;
bool buttonPressFlag = false;
bool buttonHeldFlag = false;

int pageIndex = 0;

enum State {
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

void setup() {
    Serial.begin(115200);
    Serial.println("ESPino32 Multi-Touch Sensor Ready!");
    // Create FreeRTOS tasks
    xTaskCreate(touchTask, "TouchTask", 2048, NULL, 1, NULL);
    lcd.init();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("WELCOME");
    lcd.setCursor(0, 1);
    lcd.print("Initializing...");
    delay(500);
    currentState = HOME;
}

void loop() {

    switch (currentState){
        case HOME:{
            Serial.println("HOME malaew");
            //display homepage
            lcdHomepage();
            if (buttonHeldFlag == true) {
                //Serial.println("button held ma if laew jing jing na");
                currentState = MODE_SELECT;
            }
            }
            break;
        case MODE_SELECT:{
            State currentStateTemp = MODE_SELECT;
            int ngo = 0;
            Serial.println("MODE_SELECT ma leaw");
                while(ngo ==0){
                    if(rotateRightFlag){
                        pageIndex++;
                        if(pageIndex % 2 == 1){
                            currentStateTemp = SELECT_CHANNEL_MODE;
                            Serial.println("SELECT_CHANNEL_MODE ma leaw ror");
                        }
                        else if (pageIndex % 2 == 0)
                        {
                            currentStateTemp = ON_OFF_TIMER_MODE;
                            Serial.println("ON_OFF_TIMER_MODE ma leaw ror");
                        }
                        
                    }
                    else if(rotateLeftFlag){
                        pageIndex--;
                        if(pageIndex % 2 == 1){
                            currentStateTemp = SELECT_CHANNEL_MODE;
                            Serial.println("SELECT_CHANNEL_MODE ma leaw rol");
                        }
                        else if (pageIndex % 2 == 0)
                        {
                            currentStateTemp = ON_OFF_TIMER_MODE;
                            Serial.println("ON_OFF_TIMER_MODE ma leaw rol");
                        }
                    }
                    else if (buttonPressFlag){
                        ngo = 1;
                        currentState = currentStateTemp;
                        Serial.println(currentState);
                        pageIndex = 0; //reset page index
                    }
                }
            }
            break;
        case SELECT_CHANNEL_MODE:{
            State currentStateTemp1 = SELECT_CHANNEL_MODE;
            int ngo1 = 0;
            Serial.println("SELECT_CHANNEL_MODE ma leaw");
            while (ngo1 == 0){
                if(rotateRightFlag){
                    //increment index
                    pageIndex++;
                    if (pageIndex % 5 == 0){
                        selectedChannel1();
                        currentStateTemp1 = ADJUST_INTENSITY__CH1_MODE;
                    }
                    else if (pageIndex % 5 == 1){
                        selectedChannel2();
                        currentStateTemp1 = ADJUST_INTENSITY__CH2_MODE;
                    }
                    else if (pageIndex % 5 == 2){
                        selectedChannel3();
                        currentStateTemp1 = ADJUST_INTENSITY__CH3_MODE;
                    }
                    else if (pageIndex % 5 == 3){
                        selectedChannel4();
                        currentStateTemp1 = ADJUST_INTENSITY__CH4_MODE;
                    }
                    else if (pageIndex % 5 == 4){
                        selectedChannel5();
                        currentStateTemp1 = ADJUST_INTENSITY__CH5_MODE;
                    }
                }
                else if(rotateLeftFlag){
                    //decrement index
                    pageIndex--;
                    if (pageIndex % 5 == 0){
                        selectedChannel1();
                        currentStateTemp1 = ADJUST_INTENSITY__CH1_MODE;
                    }
                    else if (pageIndex % 5 == 1){
                        selectedChannel2();
                        currentStateTemp1 = ADJUST_INTENSITY__CH2_MODE;
                    }
                    else if (pageIndex % 5 == 2){
                        selectedChannel3();
                        currentStateTemp1 = ADJUST_INTENSITY__CH3_MODE;
                    }
                    else if (pageIndex % 5 == 3){
                        selectedChannel4();
                        currentStateTemp1 = ADJUST_INTENSITY__CH4_MODE;
                    }
                    else if (pageIndex % 5 == 4){
                        selectedChannel5();
                        currentStateTemp1 = ADJUST_INTENSITY__CH5_MODE;
                    }
                }
                else if (buttonPressFlag){
                    ngo1 = true;
                    currentState = currentStateTemp1;
                    Serial.println(currentState);
                }
            }
        }
        break;
        case ADJUST_INTENSITY__CH1_MODE:{
            Serial.println("ADJUST_INTENSITY__CH1_MODE ma leaw");
        }
        break;
        case ADJUST_INTENSITY__CH2_MODE:{
            Serial.println("ADJUST_INTENSITY__CH2_MODE ma leaw");
        }
        break;
        case ADJUST_INTENSITY__CH3_MODE:{
            Serial.println("ADJUST_INTENSITY__CH3_MODE ma leaw");
        }
        break;
        case ADJUST_INTENSITY__CH4_MODE:{
            Serial.println("ADJUST_INTENSITY__CH4_MODE ma leaw");
        }
        break;
        case ADJUST_INTENSITY__CH5_MODE:{
            Serial.println("ADJUST_INTENSITY__CH5_MODE ma leaw");
        }
        break;
        
        case ON_OFF_TIMER_MODE:{
            Serial.println("ON_OFF_TIMER_MODE ma leaw");
        }
            break;
    }
    delay(300);
}

void touchTask(void *pvParameters) {
    for (;;) {
        int touch1 = touchRead(TOUCH_PIN_1); // Read GPIO 32
        if (touch1 < TOUCH_THRESHOLD) {
            rotateRightFlag = true;
            Serial.println("TOUCH_PIN_1 touched rotating right");
        } else {
            rotateRightFlag = false;
        }
        vTaskDelay(pdMS_TO_TICKS(100)); // Delay for 500ms

        int touch2 = touchRead(TOUCH_PIN_2); // Read GPIO 33
        if (touch2 < TOUCH_THRESHOLD) {
            rotateLeftFlag = true;
            Serial.println("TOUCH_PIN_2 touched rotating left");
        } else {
            rotateLeftFlag = false;
        }
        vTaskDelay(pdMS_TO_TICKS(100)); // Delay for 500ms

        int touch3 = touchRead(TOUCH_PIN_3); // Read GPIO 12
        if (touch3 < TOUCH_THRESHOLD) {
            buttonPressFlag = true;
            Serial.println("TOUCH_PIN_3 touched button pressed");
        } else {
            buttonPressFlag = false;
        }
        vTaskDelay(pdMS_TO_TICKS(100)); // Delay for 500ms

        int touch4 = touchRead(TOUCH_PIN_4); // Read GPIO 14
        if (touch4 < TOUCH_THRESHOLD) {
            buttonHeldFlag = true;
            Serial.println("TOUCH_PIN_4 touched button held");
        } else {
            buttonHeldFlag = false;
        }
        vTaskDelay(pdMS_TO_TICKS(100)); // Delay for 500ms
    }
}

void lcdHomepage(){
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("HOMEPAGE");
    lcd.setCursor(0, 1);
    lcd.print("Time: ");
    lcd.print("13:00");
}

// Layer 2
void selectChannel(){
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("<SELECT CHANNEL MODE>");
    //lcd.print(channel);
}

// Layer 3
void onofftimer(){
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("<ON/OFF TIMER>");
}

// Select channel
void selectedChannel1(){
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Channel 1");
    lcd.setCursor(0, 1);
    lcd.print("Intensity: ");
    //lcd.print(intensity);
}
void selectedChannel2(){
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Channel 2");
    lcd.setCursor(0, 1);
    lcd.print("Intensity: ");
    //lcd.print(intensity);
}
void selectedChannel3(){
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Channel 3");
    lcd.setCursor(0, 1);
    lcd.print("Intensity: ");
    //lcd.print(intensity);
}
void selectedChannel4(){
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Channel 4");
    lcd.setCursor(0, 1);
    lcd.print("Intensity: ");
    //lcd.print(intensity);
}
void selectedChannel5(){
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Channel 5");
    lcd.setCursor(0, 1);
    lcd.print("Intensity: ");
    //lcd.print(intensity);
}