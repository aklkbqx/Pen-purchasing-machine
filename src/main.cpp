// กำหนดค่าของ Token และ Template ID สำหรับการเชื่อมต่อกับ LINE Notify และ Blynk
#define LINE_TOKEN "BAwPVl5Qq41LhakbJLOsgwCp8bidWP6tJzrf5rBgFzS"
#define BLYNK_AUTH_TOKEN "vN8aqkSZeKmBAhLfyZfTXUIBeHg16I1n"
#define BLYNK_TEMPLATE_ID "TMPL6burwBeTL"
#define BLYNK_TEMPLATE_NAME "PenPurchasingMachine"

// รวมไลบรารีต่าง ๆ ที่จำเป็นสำหรับการทำงานของเครื่องขายปากกา
#include <Arduino.h>
#include <WiFi.h>
#include <ESP32Servo.h>
#include <Wire.h>
#include <LiquidCrystal.h>
#include <WiFiManager.h>
#include <HTTPClient.h>
#include <BlynkSimpleEsp32.h>

// กำหนดพินสำหรับการเชื่อมต่อกับจอ LCD
const int rs = 22, en = 21, d4 = 18, d5 = 17, d6 = 16, d7 = 15;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7); // สร้างอ็อบเจ็กต์ LCD

// กำหนดชื่อและรหัสผ่านของ WiFi ที่จะใช้เชื่อมต่อ
const char *WIFI_NAME = "!PEN PURCHASING MACHINE";
const char *WIFI_PASSWORD = "11111111";

// กำหนดพินและตัวแปรสำหรับการนับจำนวนเงินที่รับจากเครื่องตรวจเหรียญ
const int coinValidatorPin = 13;
volatile int totalAmount = 0;
volatile unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 200;
volatile int pulseCount = 0;
volatile int pens = 0;
const unsigned long calculationDelay = 500;
const int pricePen = 8; // กำหนดราคาปากกา

// ตัวแปรที่จะกำหนดจำนวนทั้งหมดที่ปากกาใส่ได้
int amountBluePen = 10;
int amountRedPen = 10;
int bluePenPurchased = 0;
int redPenPurchased = 0;
unsigned long previousMillisPen = 0;
const long intervalPen = 5000; // กำหนดช่วงเวลาในการส่งข้อมูลเกี่ยวกับการขายปากกา

// กำหนดพินและค่าต่าง ๆ สำหรับการควบคุมเซอร์โวมอเตอร์
Servo servo1;
Servo servo2;

// กำหนดขา pin servo
const int servoPin1 = 25;
const int servoPin2 = 26;

// ค่าองศาของ Servo
const int RIGHT = 0;
const int LEFT = 180;
const int STOP = 90;
// จำนวนการ delay ของ servo
const int DELAY_SERVO1 = 1000;
const int DELAY_SERVO2 = 1000;

// กำหนดพินสำหรับปุ่มรับปากกาสีน้ำเงินและสีแดง
const int buttonGetBluePen = 32;
const int buttonGetRedPen = 33;
bool lastState_buttonGetBluePen = false;
bool lastState_buttonGetRedPen = false;

// กำหนดพินสำหรับรีเลย์ที่ควบคุมเครื่องจ่ายเหรียญและระบบตรวจเหรียญ
const int relayPin3V = 27;
bool relayState = false;
const int relayPin12V = 14;
bool relayCoinValidatorState = false;

// ทำการประกาศฟังชันที่จะทำการเรียกใช้ขึ้นมาก่อน
void IRAM_ATTR doCounter();
void updateDisplay();                                          // ฟังก์ชันอัปเดตหน้าจอแสดงผล
void refillDisplay();                                          // ฟังก์ชันแจ้งให้เติมปากกา
void calculateAmount();                                        // ฟังก์ชันคำนวณจำนวนเงินจากการนับพัลส์
void purchasPen(String PenColor);                              // ฟังก์ชันจำหน่ายปากกา
void sendLineNotify(String message, String imageUrl = "");     // ฟังก์ชันส่งแจ้งเตือนไปที่ LINE Notif
void moveServo(String servoName, int targetDeg, int duration); // ฟังก์ชันควบคุมการหมุนของเซอร์โวมอเตอร์
void checkbuttonPurchase();                                    // ฟังก์ชันตรวจสอบสถานะปุ่ม
void releaseBluePen();                                         // ฟังก์ชันจำหน่ายปากกาสีน้ำเงิน
void releaseRedPen();
void disconnectCoinValidator();
void connectCoinValidator();

// WiFiManager ใช้สำหรับการจัดการ WiFi
WiFiManager wm;
unsigned long connectStartTime;                // ตัวแปรจับเวลาการเชื่อมต่อ WiFi
const unsigned long CONNECTION_TIMEOUT = 5000; // กำหนดเวลาการเชื่อมต่อ WiFi (5 วินาที)

void setup()
{
  Serial.begin(115200);
  lcd.begin(16, 2);
  lcd.backlight();

  pinMode(coinValidatorPin, INPUT_PULLUP);
  disconnectCoinValidator();

  pinMode(relayPin12V, OUTPUT);
  digitalWrite(relayPin12V, LOW);

  lcd.setCursor(0, 0);
  lcd.print("System Starting.");
  delay(500);
  lcd.clear();

  lcd.setCursor(1, 0);
  lcd.print("Please Connect");
  lcd.setCursor(5, 1);
  lcd.print("WiFi...");
  delay(1000);
  lcd.clear();

  bool res;

  IPAddress staticIP(192, 168, 4, 1);
  IPAddress gateway(192, 168, 4, 1);
  IPAddress subnet(255, 255, 255, 0);
  wm.setAPStaticIPConfig(staticIP, gateway, subnet);

  lcd.clear();
  lcd.setCursor(6, 0);
  lcd.print("WiFi");
  lcd.setCursor(2, 1);
  lcd.print("Connecting..");
  connectStartTime = millis();

  wm.setTimeout(30);

  if (!wm.autoConnect(WIFI_NAME, WIFI_PASSWORD))
  {
    unsigned long elapsedTime = millis() - connectStartTime;
    if (elapsedTime >= CONNECTION_TIMEOUT)
    {
      Serial.println("Failed to connect within 30 seconds");
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("AP Mode Active");
      lcd.setCursor(0, 1);
      lcd.print("Connect to WiFi");
      delay(1000);
      ESP.restart();
    }
  }
  else
  {
    sendLineNotify("เชื่อมต่อกับเครื่องขายปากกาสำเร็จแล้ว! ✅", "https://lh3.googleusercontent.com/proxy/PuikqzwSBdertvKVZSgkEWbpwbt8eB-fZHFc8RXMAozAyvFRFLvS3BVBwzzcYqcNMYo_pGA2PmsBEe0yHYK8ykXZB_1d1Jwi7Le7TYhZn5b-D3mXud9DVVUJ_IIsiEE");
    Serial.println("เชื่อมต่อสำเร็จแล้ว!");
    lcd.clear();
    lcd.setCursor(6, 0);
    lcd.print("WiFi");
    lcd.setCursor(3, 1);
    lcd.print("Connected!");
    Blynk.config(BLYNK_AUTH_TOKEN);
    delay(2000);
  }

  pinMode(relayPin3V, OUTPUT);
  digitalWrite(relayPin3V, HIGH);
  digitalWrite(relayPin12V, HIGH);

  servo1.attach(servoPin1);
  servo2.attach(servoPin2);

  pinMode(buttonGetBluePen, INPUT_PULLUP);
  pinMode(buttonGetRedPen, INPUT_PULLUP);

  updateDisplay();

  Blynk.virtualWrite(V5, 1);

  delay(2000);
  connectCoinValidator();
}

void loop()
{
  Blynk.run();

  if (!relayCoinValidatorState)
  {
    unsigned long currentMillisPen = millis();
    if (currentMillisPen - previousMillisPen >= intervalPen)
    {
      if (bluePenPurchased > 0 || redPenPurchased > 0)
      {
        String message = "";
        if (bluePenPurchased > 0)
        {
          message += "\n🔵 จำหน่ายปากกาสีน้ำเงิน " + String(bluePenPurchased) + " ด้าม\n";
        }
        if (redPenPurchased > 0)
        {
          message += "\n🔴 จำหน่ายปากกาสีแดง " + String(redPenPurchased) + " ด้าม";
        }
        sendLineNotify(message);

        bluePenPurchased = 0;
        redPenPurchased = 0;
      }
      previousMillisPen = currentMillisPen;
    }

    static unsigned long lastCalculationTime = 0;
    unsigned long currentTime = millis();

    if (pulseCount > 0 && (currentTime - lastDebounceTime) > calculationDelay)
    {
      calculateAmount();
      lastCalculationTime = currentTime;
    }

    checkbuttonPurchase();

    if (pens > 0)
    {
      digitalWrite(relayPin3V, LOW);
    }
    else
    {
      digitalWrite(relayPin3V, HIGH);
    }
  }
  else
  {
    lcd.clear();
    lcd.setCursor(2, 0);
    lcd.print("System: OFF!");
  }
}

void checkbuttonPurchase()
{
  if (digitalRead(buttonGetBluePen) == LOW && !lastState_buttonGetBluePen)
  {
    if (amountBluePen != 0)
    {
      lastState_buttonGetBluePen = true;
      purchasPen("blue");
    }
    else
    {
      refillDisplay();
    }
  }
  if (digitalRead(buttonGetRedPen) == LOW && !lastState_buttonGetRedPen)
  {
    if (amountRedPen != 0)
    {
      lastState_buttonGetRedPen = true;
      purchasPen("red");
    }
  }
  if (digitalRead(buttonGetBluePen) == HIGH)
  {
    lastState_buttonGetBluePen = false;
  }
  if (digitalRead(buttonGetRedPen) == HIGH)
  {
    lastState_buttonGetRedPen = false;
  }
}

void moveServo(String servoName, int targetDeg, int duration)
{
  Servo *servo = (servoName == "blue") ? &servo1 : &servo2;
  servo->write(targetDeg);
  delay(duration);
}

void IRAM_ATTR doCounter()
{
  unsigned long currentTime = millis();
  if ((currentTime - lastDebounceTime) > debounceDelay)
  {
    pulseCount++;
    lastDebounceTime = currentTime;
  }
}

void updateDisplay()
{
  lcd.clear();
  if (totalAmount == 0)
  {
    lcd.setCursor(2, 0);
    lcd.print("Insert Coin!");
  }
  else
  {
    lcd.setCursor(0, 0);
    lcd.print("Total: " + String(totalAmount) + " Bath.-");

    lcd.setCursor(0, 1);
    lcd.print("Pens: " + String(pens));
  }
  delay(100);
}

void refillDisplay()
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Can't buy a Pen");
  lcd.setCursor(0, 1);
  lcd.print("Please refill it");
  delay(2000);
}

void calculateAmount()
{
  if (pulseCount > 0)
  {
    Serial.print("จำนวน Pulse: " + String(pulseCount));

    Serial.print("เพิ่มเงิน: ");

    if (pulseCount == 1)
    {
      totalAmount += 1;
      Serial.println("1 บาท");
    }
    else if (pulseCount == 2)
    {
      totalAmount += 2;
      Serial.println("2 บาท");
    }
    else if (pulseCount >= 3 && pulseCount < 5)
    {
      totalAmount += 5;
      Serial.println("5 บาท");
    }
    else if (pulseCount > 4)
    {
      totalAmount += 10;
      Serial.println("10 บาท");
    }
    else
    {
      totalAmount = 0;
      Serial.println("0 บาท");
    }

    if (totalAmount / pricePen > pens)
    {
      pens = totalAmount / pricePen;
      Serial.print("จำนวน pens: " + String(pens));
    }

    pulseCount = 0;

    Serial.print("ยอดเงินรวมทั้งหมด: " + String(totalAmount) + " บาท");
    updateDisplay();
  }
}
void purchasPen(String PenColor)
{
  if (pens >= 1)
  {
    Serial.print("purches Pen!");

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("You have success");
    lcd.setCursor(0, 1);
    lcd.print("Purchased 1 pen!");
    delay(50);

    pens -= 1;
    totalAmount -= pricePen;

    if (PenColor == "blue")
    {
      releaseBluePen();
      bluePenPurchased += 1;
      amountBluePen -= 1;
      if (amountBluePen <= 5)
      {
        sendLineNotify("\n🔵 ปากกาสีน้ำเงินใกล้จะหมดแล้ว\n\nจำนวนที่เหลือ: " + String(amountBluePen) + " ด้าม");
      }
      if (amountBluePen == 0)
      {
        sendLineNotify("\n🔵 ปากกาสีน้ำเงินหมดแล้ว\n\nกรุณาเติมปากกา!!");
        refillDisplay();
      }
    }
    else if (PenColor == "red")
    {
      releaseRedPen();
      redPenPurchased += 1;
      amountRedPen -= 1;
      if (amountRedPen <= 5)
      {
        sendLineNotify("\n🔴 ปากกาสีแดงใกล้จะหมดแล้ว\n\nจำนวนที่เหลือ: " + String(amountRedPen) + " ด้าม");
      }
      if (amountRedPen == 0)
      {
        sendLineNotify("\n🔴 ปากกาสีแดงหมดแล้ว\n\nกรุณาเติมปากกา!!");
      }
    }
    previousMillisPen = millis();
    updateDisplay();
  }
  else
  {
    lcd.clear();
    lcd.setCursor(2, 0);
    lcd.print("Please insert");
    lcd.setCursor(6, 1);
    lcd.print("Coin");
    delay(2000);
    updateDisplay();
  }
}

void sendLineNotify(String message, String imageUrl)
{
  HTTPClient http;
  http.begin("https://notify-api.line.me/api/notify");
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  http.addHeader("Authorization", "Bearer " + String(LINE_TOKEN));

  String httpRequestData = "message=" + message;
  if (imageUrl != "")
  {
    httpRequestData += "&imageThumbnail=" + imageUrl + "&imageFullsize=" + imageUrl;
  }
  int httpResponseCode = http.POST(httpRequestData);

  if (httpResponseCode > 0)
  {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    String payload = http.getString();
    Serial.println(payload);
  }
  else
  {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  http.end();
}

BLYNK_WRITE(V1)
{
  int val = param.asInt();
  servo1.write(val);
}
BLYNK_WRITE(V2)
{
  int val = param.asInt();
  servo2.write(val);
}
BLYNK_WRITE(V3)
{
  if (param.asInt() == 1)
  {
    releaseBluePen();
  }
}
BLYNK_WRITE(V4)
{
  if (param.asInt() == 1)
  {
    releaseRedPen();
  }
}
BLYNK_WRITE(V5)
{
  if (!relayCoinValidatorState)
  {
    if (param.asInt() == 1)
    {
      digitalWrite(relayPin12V, LOW);
      relayCoinValidatorState = true;
      lcd.clear();
      lcd.setCursor(5, 0);
      lcd.print("System");
      lcd.setCursor(2, 1);
      lcd.print("Shutting Down.");
      delay(2000);
      totalAmount = 0;
      pens = 0;
      disconnectCoinValidator();
      delay(2000);
      connectCoinValidator();
    }
  }
  else
  {
    if (param.asInt() == 0)
    {
      digitalWrite(relayPin12V, HIGH);
      relayCoinValidatorState = false;
      lcd.clear();
      lcd.setCursor(5, 0);
      lcd.print("System");
      lcd.setCursor(2, 1);
      lcd.print("Starting....");
      delay(2000);
      updateDisplay();
      amountBluePen = 10;
      amountRedPen = 10;
    }
  }
}

void disconnectCoinValidator()
{
  detachInterrupt(digitalPinToInterrupt(coinValidatorPin));
}

void connectCoinValidator()
{
  attachInterrupt(digitalPinToInterrupt(coinValidatorPin), doCounter, FALLING);
}

void releaseBluePen()
{
  moveServo("blue", LEFT, 200);
  moveServo("blue", RIGHT, 210);
  moveServo("blue", STOP, 200);
  moveServo("blue", RIGHT, 200);
  moveServo("blue", LEFT, 210);
  moveServo("blue", STOP, 200);

  moveServo("blue", LEFT, 958);
  moveServo("blue", STOP, 200);
  moveServo("blue", LEFT, 958);
  moveServo("blue", STOP, 0);
}
void releaseRedPen()
{
  moveServo("red", LEFT, 200);
  moveServo("red", RIGHT, 210);
  moveServo("red", STOP, 200);
  moveServo("red", RIGHT, 200);
  moveServo("red", LEFT, 210);
  moveServo("red", STOP, 200);

  moveServo("red", LEFT, 985);
  moveServo("red", STOP, 200);
  moveServo("red", LEFT, 985);
  moveServo("red", STOP, 0);
}
