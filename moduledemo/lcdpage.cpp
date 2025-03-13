#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// LCD setup
LiquidCrystal_I2C lcd(0x27, 16, 2);

int channel = 1;
int onoff = 0; //off = 0, on =1
int intensity = 0;
char time;
void setup () {
    lcd.init();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("WELCOME");
    lcd.setCursor(0, 1);
    lcd.print("Initializing...");
    delay(2000);
}

void loop () {
    // Empty loop
}

void lcdHomepage(){
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("HOMEPAGE");
    lcd.setCursor(0, 1);
    lcd.print("Channel: ");
    lcd.print(channel);
}
//layer 2
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

//layer 3
//select channel
void selectedChannel1(){
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Channel 1");
    lcd.setCursor(0, 1);
    lcd.print("Intensity: ");
    lcd.print(channel);
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
//on off timer
void onTimer(){
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("ON TIMER");
    lcd.setCursor(0, 1);
    lcd.print("Time: ");
    lcd.print(time);
}
void offTimer(){
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("OFF TIMER");
    lcd.setCursor(0, 1);
    lcd.print("Time: ");
    lcd.print(time);
}