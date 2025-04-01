/***********************************************************************************************************
การรับค่าจาก BLE ของฝั่ง  Client
โดยจะเป็น array โครงสร้างดังนี้
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
bool ledTimeSet = false;  // Flag สำหรับตรวจสอบว่าตั้งค่าหรือยัง

ErriezDS3231 rtc;
BLEServer *pServer = NULL;
BLECharacteristic *pCharacteristic = NULL;
bool deviceConnected = false;
void rtcInit();

class MyServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
        deviceConnected = true;
        Serial.println("Client Connected");
    };

    void onDisconnect(BLEServer* pServer) {
        deviceConnected = false;
        Serial.println("Client Disconnected, Restarting Advertising...");
        delay(1000);
        pServer->getAdvertising()->start();
    }
};

class MyCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
        // รับข้อมูลจาก BLE Characteristic และตรวจสอบความยาวข้อมูล
        String receivedData = pCharacteristic->getValue();
        int len = receivedData.length();

        // ตรวจสอบว่าข้อมูลต้องมีความยาว 7 ไบต์
        if (len == 7) {  
            uint8_t data[len];
            memcpy(data, receivedData.c_str(), len); // คัดลอกข้อมูลไปยังอาร์เรย์ data

            // ดึงค่าต่างๆ จากข้อมูลที่ได้รับ
            int hr = data[0];      // ชั่วโมง
            int min = data[1];     // นาที
            int set = data[4];     // กำหนดว่าเป็นการตั้งค่าเปิด (1) หรือปิด (0)
            int mode = data[6];    // โหมดของคำสั่ง 

            Serial.print("Received mode: ");
            Serial.println(mode);

            // ตรวจสอบว่า mode เป็น 2 
            if (mode == 2) {
                if (set == 1) { // ตั้งเวลาเปิด LED
                    // ตรวจสอบว่าเวลาเปิดต้องไม่ตรงกับเวลาปิด
                    if (hr == ledHourOff && min == ledMinOff) {
                        Serial.println("Error: ON time cannot be the same as OFF time!");
                        return; // ออกจากฟังก์ชันหากเวลาเปิดตรงกับเวลาปิด
                    }
                    ledHourOn = hr;
                    ledMinOn = min;
                    Serial.print("LED ON time set to: ");
                } else if (set == 0) { // ตั้งเวลาปิด LED
                    // ตรวจสอบว่าเวลาปิดต้องไม่ตรงกับเวลาเปิด
                    if (hr == ledHourOn && min == ledMinOn) {
                        Serial.println("Error: OFF time cannot be the same as ON time!");
                        return; // ออกจากฟังก์ชันหากเวลาปิดตรงกับเวลาเปิด
                    }
                    ledHourOff = hr;
                    ledMinOff = min;
                    Serial.print("LED OFF time set to: ");
                }
                // แสดงเวลาที่ตั้งค่า
                Serial.printf("%02d:%02d\n", hr, min);

                // ตั้งค่าให้ LED ทำงานตามกำหนดเวลา
                ledTimeSet = true;
            } else {
                Serial.println("Invalid mode received!"); // แจ้งเตือนหาก mode ไม่ถูกต้อง
            }
        } else {
            Serial.println("Invalid data length!"); // แจ้งเตือนหากข้อมูลที่ได้รับมีความยาวไม่ถูกต้อง
        }
    }
};

void setup() { 
    Serial.begin(115200);
    Serial.println(F("\nErriez DS3231 RTC Set Time and LED Control with BLE\n"));

    // เริ่มต้น I2C
    Wire.begin();
    Wire.setClock(100000);

    // เรียกใช้งาน RTC
    rtcInit();

    // ตั้งค่าโหมดขา LED
    pinMode(LED_PIN, OUTPUT);

    // เริ่มต้น BLE
    BLEDevice::init("ESP32_BLE");
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    BLEService *pService = pServer->createService(BLEUUID(SERVICE_UUID));
    pCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID_WRITE,
        BLECharacteristic::PROPERTY_WRITE
    );
    
    pCharacteristic->setCallbacks(new MyCallbacks());
    pService->start();

    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);
    pAdvertising->setMinPreferred(0x12);
    BLEDevice::startAdvertising();

    Serial.println("Waiting for client connection...");
}

void loop() {
    // สร้างตัวแปรโครงสร้าง tm เพื่อเก็บข้อมูลวันที่และเวลา
    struct tm dt;
    
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

        bool ledState = isLedOn(currentHour, currentMinute, ledHourOn, ledMinOn, ledHourOff, ledMinOff);
        digitalWrite(LED_PIN, ledState ? HIGH : LOW);

        Serial.print("LED State: ");
        Serial.println(ledState ? "ON" : "OFF");
    } else {
        Serial.println("Waiting for LED time setup...");
    }

    // ตัวแปร static ใช้เก็บสถานะการโฆษณา BLE
    static bool isAdvertising = true;
    
    // ตรวจสอบว่าไม่มีอุปกรณ์เชื่อมต่อและ BLE ยังไม่ได้เริ่มโฆษณาใหม่
    if (!deviceConnected && !isAdvertising) {
        Serial.println("Restarting BLE Advertising..."); // แจ้งเตือนใน Serial Monitor
        BLEDevice::startAdvertising(); // เริ่มโฆษณา BLE ใหม่
        isAdvertising = true; // อัปเดตสถานะว่ากำลังโฆษณา
    } 
    // ตรวจสอบว่ามีอุปกรณ์เชื่อมต่อแล้ว
    else if (deviceConnected) {
        isAdvertising = false; // หยุดโฆษณา BLE
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