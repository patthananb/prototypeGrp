/***********************************************************************************************************
การรับค่าจาก BLE ของฝั่ง  Client
โดย array มีโครงสร้างดังนี้
[hr,min,0,0,set,0,mode]
set => 1 is on
    => 0 is off
mode ของ on/off คือ  2 
เช่น [13,20,0,0,1,0,2] ก็คือ mode on/off โดย set on ที่เวลา 13.20
***********************************************************************************************************/
#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <ErriezDS3231.h>
#include <Wire.h>

#define SERVICE_UUID "12345678-1234-5678-1234-56789abcdef0"
#define CHARACTERISTIC_UUID_WRITE "87654321-4321-6789-4321-abcdef012345"
#define LED_PIN 13  

int ledHourOn = -1, ledMinOn = -1;  // ใช้ค่าเริ่มต้น -1 เพื่อบอกว่ายังไม่ได้ตั้งค่า
int ledHourOff = -1, ledMinOff = -1;
bool ledTimeSet = false;  // Flag สำหรับตรวจสอบว่ากำหนดค่าเวลาเปิด/ปิดหรือยัง

ErriezDS3231 rtc;

// Define BLE characteristics
BLEServer *pServer = NULL;
BLECharacteristic *pCharacteristic = NULL;
bool deviceConnected = false;
void rtcInit();
bool isLedOn(int currentHour, int currentMinute, int onHour, int onMinute, int offHour, int offMinute);
bool ledState;

// ฟังก์ชัน callback สำหรับสถานะการเชื่อมต่อ
class MyServerCallbacks  : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true; // ตั้งค่าสถานะว่าเชื่อมต่อแล้ว
    Serial.println("Device connected!");
  }

  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false; // ตั้งค่าสถานะว่าไม่ได้เชื่อมต่อ
    Serial.println("Device disconnected!");
  }
};

class MyCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
        String receivedData = pCharacteristic->getValue();
        if (receivedData.length() != 7) {
            Serial.println("Invalid data length!");
            return;
        }

        uint8_t data[7];
        memcpy(data, receivedData.c_str(), 7);

        int hr = data[0];
        int min = data[1];
        int set = data[4];
        int mode = data[6];

        Serial.printf("Received mode: %d\n", mode);

        if (mode == 2) {
            if (set == 1) {
                if (hr == ledHourOff && min == ledMinOff) {
                    Serial.println("Error: ON time and OFF time cannot be the same!");
                    return;
                    }
                ledHourOn = hr;
                ledMinOn = min;
                Serial.printf("LED ON time set to: %02d:%02d\n", hr, min);
            } else if (set == 0) {
                if (hr == ledHourOn && min == ledMinOn) {
                    Serial.println("Error: ON time and OFF time cannot be the same!");
                    return;
                    }
                ledHourOff = hr;
                ledMinOff = min;
                Serial.printf("LED OFF time set to: %02d:%02d\n", hr, min);
            }
          ledTimeSet = true;
        } else {
            Serial.println("Invalid mode received!");
            }
    }
};

void setup() { 
    Serial.begin(115200);

    // เริ่มต้น I2C
    Wire.begin();
    Wire.setClock(100000);

    // เรียกใช้งาน RTC
    rtcInit();
    struct tm dt = {0};
    if (rtc.read(&dt)) {
        Serial.printf("RTC Time: %04d-%02d-%02d %02d:%02d:%02d\n",
                  dt.tm_year + 1900, dt.tm_mon + 1, dt.tm_mday,
                  dt.tm_hour, dt.tm_min, dt.tm_sec);
    } else {
        Serial.println("Failed to read RTC");
    }
    // ตั้งค่าโหมดขา LED
    pinMode(LED_PIN, OUTPUT);

    // เริ่มต้น BLE
    BLEDevice::init("ESP32_BLE");
    // สร้าง BLE server และตั้งค่า callback สำหรับสถานะการเชื่อมต่อ
    BLEServer *pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    // สร้าง BLE service
    BLEService *pService = pServer->createService(SERVICE_UUID);

    // สร้าง BLE characteristic ที่สามารถอ่าน เขียน และแจ้งเตือน
pCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID_WRITE,
    BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
);

  pCharacteristic->setCallbacks(new MyCallbacks());
  // เริ่มต้นการทำงานของ service และเริ่มโฆษณา (advertise)
  pService->start();
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
}

void loop() {
    struct tm dt = {0};
    // อ่านค่าจาก RTC ถ้าล้มเหลวให้แสดงข้อความและออกจากฟังก์ชัน
    if (!rtc.read(&dt)) {
        Serial.println("Failed to read RTC"); // แจ้งเตือนว่าอ่านค่า RTC ไม่สำเร็จ
        delay(1000); 
        return; 
    }

    // ดึงค่าชั่วโมงและนาทีปัจจุบันจากโครงสร้างเวลา
    int currentHour = dt.tm_hour;
    int currentMinute = dt.tm_min;
    Serial.printf("Current Time: %02d:%02d\n", currentHour, currentMinute);

    if (ledTimeSet) {  // ตรวจสอบว่ามีการตั้งค่าเวลาหรือยัง
        Serial.printf("ON Time: %02d:%02d\n", ledHourOn, ledMinOn);
        Serial.printf("OFF Time: %02d:%02d\n", ledHourOff, ledMinOff);

        ledState = isLedOn(currentHour, currentMinute, ledHourOn, ledMinOn, ledHourOff, ledMinOff);
        digitalWrite(LED_PIN, ledState ? HIGH : LOW);

        Serial.print("LED State: ");
        Serial.println(ledState ? "ON" : "OFF");
    } else {
        Serial.println("Waiting for LED time setup...");
    }
    delay(1000);
}

bool isLedOn(int currentHour, int currentMinute, int onHour, int onMinute, int offHour, int offMinute) {
    // แปลงเวลาให้เป็นจำนวนนาทีตั้งแต่เที่ยงคืน
    int currentMinutes = currentHour * 60 + currentMinute;
    int onMinutes = onHour * 60 + onMinute;
    int offMinutes = offHour * 60 + offMinute;

    // ตรวจสอบว่าห้ามตั้งเวลาเปิดและปิดเป็นค่าเดียวกัน
    if (onMinutes == offMinutes) {
        Serial.println("Error: ON and OFF times cannot be the same!");
        return false;
    }

    if (onMinutes < offMinutes) {  
        // กรณีช่วงเวลาเปิด-ปิดอยู่ภายในวันเดียวกัน (เช่น 08:00 - 20:00)
        return (currentMinutes >= onMinutes && currentMinutes < offMinutes);
    } else {  
        // กรณีช่วงเวลาเปิด-ปิดคร่อมข้ามเที่ยงคืน (เช่น 22:00 - 06:00)
        return (currentMinutes >= onMinutes || currentMinutes < offMinutes);
    }
}

void rtcInit() {
    while (!rtc.begin()) {
        Serial.println(F("RTC not found"));
        delay(3000);
    }
    rtc.clockEnable(true);
    rtc.setSquareWave(SquareWaveDisable);
}