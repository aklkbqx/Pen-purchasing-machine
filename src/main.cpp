// กำหนดค่าของ Token และ Template ID สำหรับการเชื่อมต่อกับ LINE Notify และ Blynk
#define LINE_TOKEN "BAwPVl5Qq41LhakbJLOsgwCp8bidWP6tJzrf5rBgFzS" // กำหนดค่า Token สำหรับ LINE Notify เพื่อใช้ในการส่งข้อความ
#define BLYNK_AUTH_TOKEN "vN8aqkSZeKmBAhLfyZfTXUIBeHg16I1n"      // กำหนดค่า Authentication Token สำหรับ Blynk
#define BLYNK_TEMPLATE_ID "TMPL6burwBeTL"                        // กำหนดค่า Template ID สำหรับ Blynk
#define BLYNK_TEMPLATE_NAME "PenPurchasingMachine"               // กำหนดชื่อ Template สำหรับ Blynk

// รวมไลบรารีต่าง ๆ ที่จำเป็นสำหรับการทำงานของเครื่องขายปากกา
#include <Arduino.h>          // ไลบรารีหลักสำหรับ Arduino
#include <WiFi.h>             // ไลบรารีสำหรับการเชื่อมต่อ WiFi
#include <ESP32Servo.h>       // ไลบรารีสำหรับควบคุมเซอร์โวมอเตอร์
#include <Wire.h>             // ไลบรารีสำหรับการสื่อสาร I2C
#include <LiquidCrystal.h>    // ไลบรารีสำหรับการควบคุมจอ LCD
#include <WiFiManager.h>      // ไลบรารีสำหรับการจัดการการเชื่อมต่อ WiFi
#include <HTTPClient.h>       // ไลบรารีสำหรับการส่ง HTTP requests
#include <BlynkSimpleEsp32.h> // ไลบรารีสำหรับเชื่อมต่อกับ Blynk

// กำหนดพินสำหรับการเชื่อมต่อกับจอ LCD
const int rs = 22, en = 21, d4 = 18, d5 = 17, d6 = 16, d7 = 15; // กำหนดพินที่เชื่อมต่อกับจอ LCD
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);                      // สร้างอ็อบเจ็กต์ LCD ด้วยพินที่กำหนด

// กำหนดชื่อและรหัสผ่านของ WiFi ที่จะใช้เชื่อมต่อ
const char *WIFI_NAME = "!PEN PURCHASING MACHINE"; // ชื่อ WiFi ที่จะเชื่อมต่อ
const char *WIFI_PASSWORD = "11111111";            // รหัสผ่าน WiFi

// กำหนดพินและตัวแปรสำหรับการนับจำนวนเงินที่รับจากเครื่องตรวจเหรียญ
const int coinValidatorPin = 13;             // พินที่เชื่อมต่อกับเครื่องตรวจเหรียญ
volatile int totalAmount = 0;                // ตัวแปรเก็บยอดรวมเงิน
volatile unsigned long lastDebounceTime = 0; // ตัวแปรสำหรับการป้องกันการกระตุกของสัญญาณ
const unsigned long debounceDelay = 200;     // ระยะเวลาในการป้องกันการกระตุก (200 มิลลิวินาที)
volatile int pulseCount = 0;                 // จำนวนพัลส์ที่นับได้จากเครื่องตรวจเหรียญ
volatile int pens = 0;                       // จำนวนปากกาที่สามารถซื้อได้
const unsigned long calculationDelay = 500;  // เวลาที่ใช้ในการคำนวณจำนวนเงิน
const int pricePen = 8;                      // ราคาของปากกา

// ตัวแปรที่จะกำหนดจำนวนทั้งหมดที่ปากกาใส่ได้
int amountBluePen = 10;              // จำนวนปากกาสีน้ำเงินที่มีอยู่
int amountRedPen = 10;               // จำนวนปากกาสีแดงที่มีอยู่
int bluePenPurchased = 0;            // จำนวนปากกาสีน้ำเงินที่ซื้อไปแล้ว
int redPenPurchased = 0;             // จำนวนปากกาสีแดงที่ซื้อไปแล้ว
unsigned long previousMillisPen = 0; // ตัวแปรสำหรับการจับเวลาในการส่งข้อมูลเกี่ยวกับการขายปากกา
const long intervalPen = 5000;       // ช่วงเวลาในการส่งข้อมูลเกี่ยวกับการขายปากกา (5 วินาที)

// กำหนดพินและค่าต่าง ๆ สำหรับการควบคุมเซอร์โวมอเตอร์
Servo servo1; // สร้างอ็อบเจ็กต์เซอร์โวมอเตอร์ 1
Servo servo2; // สร้างอ็อบเจ็กต์เซอร์โวมอเตอร์ 2

const int servoPin1 = 25; // พินที่เชื่อมต่อกับเซอร์โวมอเตอร์ 1
const int servoPin2 = 26; // พินที่เชื่อมต่อกับเซอร์โวมอเตอร์ 2

const int RIGHT = 0;           // มุมการหมุนเซอร์โวมอเตอร์ไปทางขวา
const int LEFT = 180;          // มุมการหมุนเซอร์โวมอเตอร์ไปทางซ้าย
const int STOP = 90;           // มุมการหยุดการหมุนของเซอร์โวมอเตอร์
const int DELAY_SERVO1 = 1000; // เวลาในการหน่วงของเซอร์โวมอเตอร์ 1
const int DELAY_SERVO2 = 1000; // เวลาในการหน่วงของเซอร์โวมอเตอร์ 2

// กำหนดพินสำหรับปุ่มรับปากกาสีน้ำเงินและสีแดง
const int buttonGetBluePen = 32;         // พินที่เชื่อมต่อกับปุ่มรับปากกาสีน้ำเงิน
const int buttonGetRedPen = 33;          // พินที่เชื่อมต่อกับปุ่มรับปากกาสีแดง
bool lastState_buttonGetBluePen = false; // สถานะล่าสุดของปุ่มรับปากกาสีน้ำเงิน
bool lastState_buttonGetRedPen = false;  // สถานะล่าสุดของปุ่มรับปากกาสีแดง

// กำหนดพินสำหรับรีเลย์ที่ควบคุมเครื่องจ่ายเหรียญและระบบตรวจเหรียญ
const int relayPin3V = 27;            // พินที่ควบคุมรีเลย์ 3V
bool relayState = false;              // สถานะของรีเลย์ 3V
const int relayPin12V = 14;           // พินที่ควบคุมรีเลย์ 12V
bool relayCoinValidatorState = false; // สถานะของรีเลย์ 12V

// ทำการประกาศฟังชันที่จะทำการเรียกใช้ขึ้นมาก่อน
void IRAM_ATTR doCounter();                                    // ฟังก์ชันที่ใช้ในการนับพัลส์จากเครื่องตรวจเหรียญ
void updateDisplay();                                          // ฟังก์ชันอัปเดตหน้าจอแสดงผล
void refillDisplay();                                          // ฟังก์ชันแจ้งให้เติมปากกา
void calculateAmount();                                        // ฟังก์ชันคำนวณจำนวนเงินจากการนับพัลส์
void purchasPen(String PenColor);                              // ฟังก์ชันจำหน่ายปากกา
void sendLineNotify(String message, String imageUrl = "");     // ฟังก์ชันส่งแจ้งเตือนไปที่ LINE Notify
void moveServo(String servoName, int targetDeg, int duration); // ฟังก์ชันควบคุมการหมุนของเซอร์โวมอเตอร์
void checkbuttonPurchase();                                    // ฟังก์ชันตรวจสอบสถานะปุ่ม
void releaseBluePen();                                         // ฟังก์ชันจำหน่ายปากกาสีน้ำเงิน
void releaseRedPen();                                          // ฟังก์ชันจำหน่ายปากกาสีแดง
void disconnectCoinValidator();                                // ฟังก์ชันตัดการเชื่อมต่อเครื่องตรวจเหรียญ
void connectCoinValidator();                                   // ฟังก์ชันเชื่อมต่อเครื่องตรวจเหรียญ

// WiFiManager ใช้สำหรับการจัดการ WiFi
WiFiManager wm;                                // สร้างอ็อบเจ็กต์ WiFiManager
unsigned long connectStartTime;                // ตัวแปรจับเวลาการเชื่อมต่อ WiFi
const unsigned long CONNECTION_TIMEOUT = 5000; // กำหนดเวลาการเชื่อมต่อ WiFi (5 วินาที)

void setup()
{
  Serial.begin(115200); // เริ่มต้นการเชื่อมต่อ Serial ที่ 115200 bps
  lcd.begin(16, 2);     // เริ่มต้นการทำงานของจอ LCD ขนาด 16x2
  lcd.backlight();      // เปิดไฟแบ็คไลท์ของจอ LCD

  pinMode(coinValidatorPin, INPUT_PULLUP); // ตั้งค่าขา coinValidatorPin เป็น INPUT_PULLUP
  disconnectCoinValidator();               // เรียกใช้ฟังก์ชัน disconnectCoinValidator()

  pinMode(relayPin12V, OUTPUT);   // ตั้งค่าขา relayPin12V เป็น OUTPUT
  digitalWrite(relayPin12V, LOW); // ตั้งค่าขา relayPin12V ให้เป็น LOW

  lcd.setCursor(0, 0);           // ตั้งตำแหน่งของเคอร์เซอร์ที่แถว 0 คอลัมน์ 0
  lcd.print("System Starting."); // แสดงข้อความ "System Starting." บนจอ LCD
  delay(500);                    // รอ 500 มิลลิวินาที
  lcd.clear();                   // ล้างข้อความบนจอ LCD

  lcd.setCursor(1, 0);         // ตั้งตำแหน่งของเคอร์เซอร์ที่แถว 0 คอลัมน์ 1
  lcd.print("Please Connect"); // แสดงข้อความ "Please Connect" บนจอ LCD
  lcd.setCursor(5, 1);         // ตั้งตำแหน่งของเคอร์เซอร์ที่แถว 1 คอลัมน์ 5
  lcd.print("WiFi...");        // แสดงข้อความ "WiFi..." บนจอ LCD
  delay(1000);                 // รอ 1000 มิลลิวินาที
  lcd.clear();                 // ล้างข้อความบนจอ LCD

  bool res; // ประกาศตัวแปร bool ชื่อ res

  IPAddress staticIP(192, 168, 4, 1);                // กำหนด IP address แบบ static
  IPAddress gateway(192, 168, 4, 1);                 // กำหนด gateway address
  IPAddress subnet(255, 255, 255, 0);                // กำหนด subnet mask
  wm.setAPStaticIPConfig(staticIP, gateway, subnet); // ตั้งค่า static IP configuration

  lcd.clear();                 // ล้างข้อความบนจอ LCD
  lcd.setCursor(6, 0);         // ตั้งตำแหน่งของเคอร์เซอร์ที่แถว 0 คอลัมน์ 6
  lcd.print("WiFi");           // แสดงข้อความ "WiFi" บนจอ LCD
  lcd.setCursor(2, 1);         // ตั้งตำแหน่งของเคอร์เซอร์ที่แถว 1 คอลัมน์ 2
  lcd.print("Connecting..");   // แสดงข้อความ "Connecting.." บนจอ LCD
  connectStartTime = millis(); // บันทึกเวลาปัจจุบันเพื่อใช้ในการคำนวณเวลาเชื่อมต่อ

  wm.setTimeout(30); // ตั้งค่าเวลา timeout เป็น 30 วินาที

  if (!wm.autoConnect(WIFI_NAME, WIFI_PASSWORD)) // เชื่อมต่อ WiFi อัตโนมัติ
  {
    unsigned long elapsedTime = millis() - connectStartTime; // คำนวณเวลาที่ใช้ในการเชื่อมต่อ
    if (elapsedTime >= CONNECTION_TIMEOUT)                   // ตรวจสอบว่าเวลาเชื่อมต่อเกิน 30 วินาทีหรือไม่
    {
      Serial.println("Failed to connect within 30 seconds"); // แสดงข้อความใน Serial monitor หากเชื่อมต่อไม่สำเร็จ
      lcd.clear();                                           // ล้างข้อความบนจอ LCD
      lcd.setCursor(0, 0);                                   // ตั้งตำแหน่งของเคอร์เซอร์ที่แถว 0 คอลัมน์ 0
      lcd.print("AP Mode Active");                           // แสดงข้อความ "AP Mode Active" บนจอ LCD
      lcd.setCursor(0, 1);                                   // ตั้งตำแหน่งของเคอร์เซอร์ที่แถว 1 คอลัมน์ 0
      lcd.print("Connect to WiFi");                          // แสดงข้อความ "Connect to WiFi" บนจอ LCD
      delay(1000);                                           // รอ 1000 มิลลิวินาที
      ESP.restart();                                         // รีเซ็ต ESP
    }
  }
  else // หากเชื่อมต่อสำเร็จ
  {
    sendLineNotify("เชื่อมต่อกับเครื่องขายปากกาสำเร็จแล้ว! ✅", "รูปภาพพพพพ"); // ส่งการแจ้งเตือนผ่าน Line
    Serial.println("เชื่อมต่อสำเร็จแล้ว!");                                                                                                                                                                                              // แสดงข้อความใน Serial monitor ว่าเชื่อมต่อสำเร็จ
    lcd.clear();                                                                                                                                                                                                                    // ล้างข้อความบนจอ LCD
    lcd.setCursor(6, 0);                                                                                                                                                                                                            // ตั้งตำแหน่งของเคอร์เซอร์ที่แถว 0 คอลัมน์ 6
    lcd.print("WiFi");                                                                                                                                                                                                              // แสดงข้อความ "WiFi" บนจอ LCD
    lcd.setCursor(3, 1);                                                                                                                                                                                                            // ตั้งตำแหน่งของเคอร์เซอร์ที่แถว 1 คอลัมน์ 3
    lcd.print("Connected!");                                                                                                                                                                                                        // แสดงข้อความ "Connected!" บนจอ LCD
    Blynk.config(BLYNK_AUTH_TOKEN);                                                                                                                                                                                                 // กำหนดค่า Blynk ด้วย BLYNK_AUTH_TOKEN
    delay(2000);                                                                                                                                                                                                                    // รอ 2000 มิลลิวินาที
  }

  pinMode(relayPin3V, OUTPUT);     // ตั้งค่าขา relayPin3V เป็น OUTPUT
  digitalWrite(relayPin3V, HIGH);  // ตั้งค่าขา relayPin3V ให้เป็น HIGH
  digitalWrite(relayPin12V, HIGH); // ตั้งค่าขา relayPin12V ให้เป็น HIGH

  servo1.attach(servoPin1); // เชื่อมต่อ servo1 กับขา servoPin1
  servo2.attach(servoPin2); // เชื่อมต่อ servo2 กับขา servoPin2

  pinMode(buttonGetBluePen, INPUT_PULLUP); // ตั้งค่าขา buttonGetBluePen เป็น INPUT_PULLUP
  pinMode(buttonGetRedPen, INPUT_PULLUP);  // ตั้งค่าขา buttonGetRedPen เป็น INPUT_PULLUP

  updateDisplay(); // อัพเดตการแสดงผลบนจอ LCD

  Blynk.virtualWrite(V5, 1); // เขียนค่าลงใน Blynk virtual pin V5

  delay(2000);            // รอ 2000 มิลลิวินาที
  connectCoinValidator(); // เชื่อมต่อ coin validator
}

void loop()
{
  Blynk.run(); // เรียกใช้ฟังก์ชัน Blynk.run() เพื่ออัพเดต Blynk

  if (!relayCoinValidatorState) // หาก relayCoinValidatorState เป็น false
  {
    unsigned long currentMillisPen = millis();               // บันทึกเวลาปัจจุบัน
    if (currentMillisPen - previousMillisPen >= intervalPen) // ตรวจสอบเวลาที่ผ่านมา
    {
      if (bluePenPurchased > 0 || redPenPurchased > 0) // หากมีการซื้อปากกาสีฟ้าหรือสีแดง
      {
        String message = "";      // ประกาศตัวแปร message เป็นสตริงว่าง
        if (bluePenPurchased > 0) // หากซื้อปากกาสีฟ้า
        {
          message += "\n🔵 จำหน่ายปากกาสีน้ำเงิน " + String(bluePenPurchased) + " ด้าม\n"; // เพิ่มข้อความลงใน message
        }
        if (redPenPurchased > 0) // หากซื้อปากกาสีแดง
        {
          message += "\n🔴 จำหน่ายปากกาสีแดง " + String(redPenPurchased) + " ด้าม"; // เพิ่มข้อความลงใน message
        }
        sendLineNotify(message); // ส่งข้อความผ่าน Line Notify

        bluePenPurchased = 0; // รีเซ็ตจำนวนปากกาสีน้ำเงินที่ซื้อ
        redPenPurchased = 0;  // รีเซ็ตจำนวนปากกาสีแดงที่ซื้อ
      }
      previousMillisPen = currentMillisPen; // อัพเดตเวลาล่าสุดที่ใช้ในการตรวจสอบ
    }

    static unsigned long lastCalculationTime = 0; // ประกาศตัวแปร static สำหรับเก็บเวลาคำนวณล่าสุด
    unsigned long currentTime = millis();         // บันทึกเวลาปัจจุบัน

    if (pulseCount > 0 && (currentTime - lastDebounceTime) > calculationDelay) // หากมี pulseCount มากกว่า 0 และเวลาที่ผ่านมาเกิน delay
    {
      calculateAmount();                 // คำนวณจำนวนเงิน
      lastCalculationTime = currentTime; // อัพเดตเวลาคำนวณล่าสุด
    }

    checkbuttonPurchase(); // ตรวจสอบปุ่มสำหรับซื้อปากกา

    if (pens > 0) // หากมีปากกาคงเหลือ
    {
      digitalWrite(relayPin3V, LOW); // ตั้งค่าขา relayPin3V เป็น LOW
    }
    else
    {
      digitalWrite(relayPin3V, HIGH); // ตั้งค่าขา relayPin3V เป็น HIGH
    }
  }
  else // หาก relayCoinValidatorState เป็น true
  {
    lcd.clear();               // ล้างข้อความบนจอ LCD
    lcd.setCursor(2, 0);       // ตั้งตำแหน่งของเคอร์เซอร์ที่แถว 0 คอลัมน์ 2
    lcd.print("System: OFF!"); // แสดงข้อความ "System: OFF!" บนจอ LCD
  }
}

void checkbuttonPurchase()
{
  if (digitalRead(buttonGetBluePen) == LOW && !lastState_buttonGetBluePen) // หากปุ่มซื้อปากกาสีน้ำเงินถูกกดและสถานะปุ่มก่อนหน้าเป็น false
  {
    if (amountBluePen != 0) // หากมีปากกาสีน้ำเงินคงเหลือ
    {
      lastState_buttonGetBluePen = true; // เปลี่ยนสถานะปุ่มเป็น true
      purchasPen("blue");                // เรียกใช้ฟังก์ชัน purchasPen() สำหรับปากกาสีน้ำเงิน
    }
    else
    {
      refillDisplay(); // แสดงข้อความว่าต้องเติมปากกา
    }
  }
  if (digitalRead(buttonGetRedPen) == LOW && !lastState_buttonGetRedPen) // หากปุ่มซื้อปากกาสีแดงถูกกดและสถานะปุ่มก่อนหน้าเป็น false
  {
    if (amountRedPen != 0) // หากมีปากกาสีแดงคงเหลือ
    {
      lastState_buttonGetRedPen = true; // เปลี่ยนสถานะปุ่มเป็น true
      purchasPen("red");                // เรียกใช้ฟังก์ชัน purchasPen() สำหรับปากกาสีแดง
    }
  }
  if (digitalRead(buttonGetBluePen) == HIGH) // หากปุ่มซื้อปากกาสีน้ำเงินไม่ถูกกด
  {
    lastState_buttonGetBluePen = false; // เปลี่ยนสถานะปุ่มเป็น false
  }
  if (digitalRead(buttonGetRedPen) == HIGH) // หากปุ่มซื้อปากกาสีแดงไม่ถูกกด
  {
    lastState_buttonGetRedPen = false; // เปลี่ยนสถานะปุ่มเป็น false
  }
}

void moveServo(String servoName, int targetDeg, int duration)
{
  Servo *servo = (servoName == "blue") ? &servo1 : &servo2; // เลือกเซอร์โวที่ต้องการตามชื่อ
  servo->write(targetDeg);                                  // หมุนเซอร์โวไปยังมุมที่กำหนด
  delay(duration);                                          // รอระยะเวลาที่กำหนด
}

void IRAM_ATTR doCounter()
{
  unsigned long currentTime = millis();                 // บันทึกเวลาปัจจุบัน
  if ((currentTime - lastDebounceTime) > debounceDelay) // ตรวจสอบเวลาที่ผ่านมาเกิน delay
  {
    pulseCount++;                   // เพิ่มจำนวน pulse
    lastDebounceTime = currentTime; // อัพเดตเวลาล่าสุดที่ใช้ในการตรวจสอบ
  }
}

void updateDisplay()
{
  lcd.clear();          // ล้างข้อความบนจอ LCD
  if (totalAmount == 0) // หากยอดเงินรวมทั้งหมดเป็น 0
  {
    lcd.setCursor(2, 0);       // ตั้งตำแหน่งของเคอร์เซอร์ที่แถว 0 คอลัมน์ 2
    lcd.print("Insert Coin!"); // แสดงข้อความ "Insert Coin!" บนจอ LCD
  }
  else
  {
    lcd.setCursor(0, 0);                                    // ตั้งตำแหน่งของเคอร์เซอร์ที่แถว 0 คอลัมน์ 0
    lcd.print("Total: " + String(totalAmount) + " Bath.-"); // แสดงยอดเงินรวมทั้งหมด
    lcd.setCursor(0, 1);                                    // ตั้งตำแหน่งของเคอร์เซอร์ที่แถว 1 คอลัมน์ 0
    lcd.print("Pens: " + String(pens));                     // แสดงจำนวนปากกา
  }
  delay(100); // รอ 100 มิลลิวินาที
}

void refillDisplay()
{
  lcd.clear();                   // ล้างข้อความบนจอ LCD
  lcd.setCursor(0, 0);           // ตั้งตำแหน่งของเคอร์เซอร์ที่แถว 0 คอลัมน์ 0
  lcd.print("Can't buy a Pen");  // แสดงข้อความ "Can't buy a Pen" บนจอ LCD
  lcd.setCursor(0, 1);           // ตั้งตำแหน่งของเคอร์เซอร์ที่แถว 1 คอลัมน์ 0
  lcd.print("Please refill it"); // แสดงข้อความ "Please refill it" บนจอ LCD
  delay(2000);                   // รอ 2000 มิลลิวินาที
}

void calculateAmount()
{
  if (pulseCount > 0) // หากมี pulseCount มากกว่า 0
  {
    Serial.print("จำนวน Pulse: " + String(pulseCount)); // แสดงจำนวน pulse ใน Serial monitor
    Serial.print("เพิ่มเงิน: ");                           // แสดงข้อความ "เพิ่มเงิน: " ใน Serial monitor

    if (pulseCount == 1) // หาก pulseCount เท่ากับ 1
    {
      totalAmount += 1;        // เพิ่มเงินรวม 1 บาท
      Serial.println("1 บาท"); // แสดงข้อความ "1 บาท" ใน Serial monitor
    }
    else if (pulseCount == 2) // หาก pulseCount เท่ากับ 2
    {
      totalAmount += 2;        // เพิ่มเงินรวม 2 บาท
      Serial.println("2 บาท"); // แสดงข้อความ "2 บาท" ใน Serial monitor
    }
    else if (pulseCount >= 3 && pulseCount < 5) // หาก pulseCount มากกว่าหรือเท่ากับ 3 แต่ต่ำกว่า 5
    {
      totalAmount += 5;        // เพิ่มเงินรวม 5 บาท
      Serial.println("5 บาท"); // แสดงข้อความ "5 บาท" ใน Serial monitor
    }
    else if (pulseCount > 4) // หาก pulseCount มากกว่า 4
    {
      totalAmount += 10;        // เพิ่มเงินรวม 10 บาท
      Serial.println("10 บาท"); // แสดงข้อความ "10 บาท" ใน Serial monitor
    }
    else
    {
      totalAmount = 0;         // หากไม่ตรงเงื่อนไขใดๆ ให้ยอดเงินเป็น 0
      Serial.println("0 บาท"); // แสดงข้อความ "0 บาท" ใน Serial monitor
    }

    if (totalAmount / pricePen > pens) // หากยอดเงินหารด้วยราคาปากกามากกว่าจำนวนปากกาที่มี
    {
      pens = totalAmount / pricePen;               // อัพเดตจำนวนปากกาที่มี
      Serial.print("จำนวน pens: " + String(pens)); // แสดงจำนวนปากกาใน Serial monitor
    }

    pulseCount = 0; // รีเซ็ตจำนวน pulse

    Serial.print("ยอดเงินรวมทั้งหมด: " + String(totalAmount) + " บาท"); // แสดงยอดเงินรวมทั้งหมดใน Serial monitor
    updateDisplay();                                                 // อัพเดตการแสดงผลบนจอ LCD
  }
}

void purchasPen(String PenColor)
{
  if (pens >= 1) // หากมีปากกาคงเหลือ
  {
    Serial.print("purches Pen!"); // แสดงข้อความ "purches Pen!" ใน Serial monitor

    lcd.clear();                   // ล้างข้อความบนจอ LCD
    lcd.setCursor(0, 0);           // ตั้งตำแหน่งของเคอร์เซอร์ที่แถว 0 คอลัมน์ 0
    lcd.print("You have success"); // แสดงข้อความ "You have success" บนจอ LCD
    lcd.setCursor(0, 1);           // ตั้งตำแหน่งของเคอร์เซอร์ที่แถว 1 คอลัมน์ 0
    lcd.print("Purchased 1 pen!"); // แสดงข้อความ "Purchased 1 pen!" บนจอ LCD
    delay(50);                     // รอ 50 มิลลิวินาที

    pens -= 1;               // ลดจำนวนปากกาที่มีลง 1 อัน
    totalAmount -= pricePen; // ลดยอดเงินรวมตามราคาปากกา

    if (PenColor == "blue") // หากปากกาเป็นสีน้ำเงิน
    {
      releaseBluePen();       // เรียกใช้ฟังก์ชัน releaseBluePen()
      bluePenPurchased += 1;  // เพิ่มจำนวนปากกาสีน้ำเงินที่ขายไปแล้ว
      amountBluePen -= 1;     // ลดจำนวนปากกาสีน้ำเงินคงเหลือ
      if (amountBluePen <= 5) // หากจำนวนปากกาสีน้ำเงินคงเหลือไม่เกิน 5 อัน
      {
        sendLineNotify("\n🔵 ปากกาสีน้ำเงินใกล้จะหมดแล้ว\n\nจำนวนที่เหลือ: " + String(amountBluePen) + " ด้าม"); // ส่งการแจ้งเตือน
      }
      if (amountBluePen == 0) // หากปากกาสีน้ำเงินหมด
      {
        sendLineNotify("\n🔵 ปากกาสีน้ำเงินหมดแล้ว\n\nกรุณาเติมปากกา!!"); // ส่งการแจ้งเตือนให้เติมปากกา
        refillDisplay();                                            // แสดงข้อความว่าให้เติมปากกา
      }
    }
    else if (PenColor == "red") // หากปากกาเป็นสีแดง
    {
      releaseRedPen();       // เรียกใช้ฟังก์ชัน releaseRedPen()
      redPenPurchased += 1;  // เพิ่มจำนวนปากกาสีแดงที่ขายไปแล้ว
      amountRedPen -= 1;     // ลดจำนวนปากกาสีแดงคงเหลือ
      if (amountRedPen <= 5) // หากจำนวนปากกาสีแดงคงเหลือไม่เกิน 5 อัน
      {
        sendLineNotify("\n🔴 ปากกาสีแดงใกล้จะหมดแล้ว\n\nจำนวนที่เหลือ: " + String(amountRedPen) + " ด้าม"); // ส่งการแจ้งเตือน
      }
      if (amountRedPen == 0) // หากปากกาสีแดงหมด
      {
        sendLineNotify("\n🔴 ปากกาสีแดงหมดแล้ว\n\nกรุณาเติมปากกา!!"); // ส่งการแจ้งเตือนให้เติมปากกา
      }
    }
    previousMillisPen = millis(); // อัพเดตเวลาที่ใช้ในการจัดการปากกา
    updateDisplay();              // อัพเดตการแสดงผลบนจอ LCD
  }
  else
  {
    lcd.clear();                // ล้างข้อความบนจอ LCD
    lcd.setCursor(2, 0);        // ตั้งตำแหน่งของเคอร์เซอร์ที่แถว 0 คอลัมน์ 2
    lcd.print("Please insert"); // แสดงข้อความ "Please insert" บนจอ LCD
    lcd.setCursor(6, 1);        // ตั้งตำแหน่งของเคอร์เซอร์ที่แถว 1 คอลัมน์ 6
    lcd.print("Coin");          // แสดงข้อความ "Coin" บนจอ LCD
    delay(2000);                // รอ 2000 มิลลิวินาที
    updateDisplay();            // อัพเดตการแสดงผลบนจอ LCD
  }
}

void sendLineNotify(String message, String imageUrl)
{
  HTTPClient http;                                                     // สร้างออบเจ็กต์ HTTPClient
  http.begin("https://notify-api.line.me/api/notify");                 // กำหนด URL สำหรับการส่งการแจ้งเตือน
  http.addHeader("Content-Type", "application/x-www-form-urlencoded"); // เพิ่ม Header สำหรับการส่งข้อมูล
  http.addHeader("Authorization", "Bearer " + String(LINE_TOKEN));     // เพิ่ม Header สำหรับการตรวจสอบสิทธิ์

  String httpRequestData = "message=" + message; // สร้างข้อมูลการส่งคำขอ
  if (imageUrl != "")                            // หากมี URL ของภาพ
  {
    httpRequestData += "&imageThumbnail=" + imageUrl + "&imageFullsize=" + imageUrl; // เพิ่มข้อมูล URL ของภาพ
  }
  int httpResponseCode = http.POST(httpRequestData); // ส่งคำขอ POST และบันทึกรหัสตอบกลับ

  if (httpResponseCode > 0) // หากรหัสตอบกลับเป็นบวก
  {
    Serial.print("HTTP Response code: "); // แสดงข้อความรหัสตอบกลับใน Serial monitor
    Serial.println(httpResponseCode);     // แสดงรหัสตอบกลับ
    String payload = http.getString();    // รับข้อมูลตอบกลับ
    Serial.println(payload);              // แสดงข้อมูลตอบกลับใน Serial monitor
  }
  else
  {
    Serial.print("Error code: ");     // แสดงข้อความรหัสข้อผิดพลาดใน Serial monitor
    Serial.println(httpResponseCode); // แสดงรหัสข้อผิดพลาด
  }
  http.end(); // ปิดการเชื่อมต่อ HTTP
}

BLYNK_WRITE(V1)
{
  int val = param.asInt(); // รับค่าจาก Blynk
  servo1.write(val);       // หมุนเซอร์โว 1 ไปยังมุมที่ได้รับ
}
BLYNK_WRITE(V2)
{
  int val = param.asInt(); // รับค่าจาก Blynk
  servo2.write(val);       // หมุนเซอร์โว 2 ไปยังมุมที่ได้รับ
}
BLYNK_WRITE(V3)
{
  if (param.asInt() == 1) // หากค่าที่ได้รับเป็น 1
  {
    releaseBluePen(); // เรียกใช้ฟังก์ชัน releaseBluePen()
  }
}
BLYNK_WRITE(V4)
{
  if (param.asInt() == 1) // หากค่าที่ได้รับเป็น 1
  {
    releaseRedPen(); // เรียกใช้ฟังก์ชัน releaseRedPen()
  }
}
BLYNK_WRITE(V5)
{
  if (!relayCoinValidatorState) // หาก relayCoinValidatorState เป็น false
  {
    if (param.asInt() == 1) // หากค่าที่ได้รับเป็น 1
    {
      digitalWrite(relayPin12V, LOW); // ตั้งค่าขา relayPin12V เป็น LOW
      relayCoinValidatorState = true; // เปลี่ยนสถานะ relayCoinValidatorState เป็น true
      lcd.clear();                    // ล้างข้อความบนจอ LCD
      lcd.setCursor(5, 0);            // ตั้งตำแหน่งของเคอร์เซอร์ที่แถว 0 คอลัมน์ 5
      lcd.print("System");            // แสดงข้อความ "System" บนจอ LCD
      lcd.setCursor(2, 1);            // ตั้งตำแหน่งของเคอร์เซอร์ที่แถว 1 คอลัมน์ 2
      lcd.print("Shutting Down.");    // แสดงข้อความ "Shutting Down." บนจอ LCD
      delay(2000);                    // รอ 2000 มิลลิวินาที
      totalAmount = 0;                // รีเซ็ตยอดเงินรวม
      pens = 0;                       // รีเซ็ตจำนวนปากกา
      disconnectCoinValidator();      // เรียกใช้ฟังก์ชัน disconnectCoinValidator()
      delay(2000);                    // รอ 2000 มิลลิวินาที
      connectCoinValidator();         // เรียกใช้ฟังก์ชัน connectCoinValidator()
    }
  }
  else // หาก relayCoinValidatorState เป็น true
  {
    if (param.asInt() == 0) // หากค่าที่ได้รับเป็น 0
    {
      digitalWrite(relayPin12V, HIGH); // ตั้งค่าขา relayPin12V เป็น HIGH
      relayCoinValidatorState = false; // เปลี่ยนสถานะ relayCoinValidatorState เป็น false
      lcd.clear();                     // ล้างข้อความบนจอ LCD
      lcd.setCursor(5, 0);             // ตั้งตำแหน่งของเคอร์เซอร์ที่แถว 0 คอลัมน์ 5
      lcd.print("System");             // แสดงข้อความ "System" บนจอ LCD
      lcd.setCursor(2, 1);             // ตั้งตำแหน่งของเคอร์เซอร์ที่แถว 1 คอลัมน์ 2
      lcd.print("Starting....");       // แสดงข้อความ "Starting...." บนจอ LCD
      delay(2000);                     // รอ 2000 มิลลิวินาที
      updateDisplay();                 // อัพเดตการแสดงผลบนจอ LCD
      amountBluePen = 10;              // รีเซ็ตจำนวนปากกาสีน้ำเงิน
      amountRedPen = 10;               // รีเซ็ตจำนวนปากกาสีแดง
    }
  }
}

void disconnectCoinValidator()
{
  detachInterrupt(digitalPinToInterrupt(coinValidatorPin)); // หยุดการติดตาม interrupt สำหรับ coinValidatorPin
}

void connectCoinValidator()
{
  attachInterrupt(digitalPinToInterrupt(coinValidatorPin), doCounter, FALLING); // ตั้งค่า interrupt สำหรับ coinValidatorPin
}

void releaseBluePen()
{
  moveServo("blue", LEFT, 200);  // เคลื่อนที่เซอร์โวสีฟ้าไปทางซ้าย
  moveServo("blue", RIGHT, 210); // เคลื่อนที่เซอร์โวสีฟ้าไปทางขวา
}

void releaseRedPen()
{
  moveServo("red", LEFT, 200);  // เคลื่อนที่เซอร์โวสีแดงไปทางซ้าย
  moveServo("red", RIGHT, 210); // เคลื่อนที่เซอร์โวสีแดงไปทางขวา
}