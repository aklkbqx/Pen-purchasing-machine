#define LINE_TOKEN "BAwPVl5Qq41LhakbJLOsgwCp8bidWP6tJzrf5rBgFzS"
#define BLYNK_TEMPLATE_ID "TMPL6burwBeTL"
#define BLYNK_TEMPLATE_NAME "PenPurchasingMachine"

#include <Arduino.h>
#include <WiFi.h>
#include <ESP32Servo.h>
#include <Wire.h>
#include <LiquidCrystal.h>
#include <WiFiManager.h>
#include <HTTPClient.h>

const int rs = 22, en = 21, d4 = 18, d5 = 17, d6 = 16, d7 = 15;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

const char *WIFI_NAME = "!PEN PURCHASING MACHINE";
const char *WIFI_PASSWORD = "11111111";

const int coinValidatorPin = 13;
volatile int totalAmount = 0;
volatile unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 200;
volatile int pulseCount = 0;
volatile int pens = 0;
const unsigned long calculationDelay = 500;
const int pricePen = 7;
int currentBluePen = 6;
int currentRedPen = 6;
int bluePenPurchased = 0;
int redPenPurchased = 0;
unsigned long previousMillisPen = 0;
const long intervalPen = 5000;

Servo servo1;
Servo servo2;

const int servoPin1 = 25;
const int servoPin2 = 26;

const int LEFT = 0;
const int RIGHT = 180;
const int STOP = 90;
const int DELAY_SERVO1 = 1000;
const int DELAY_SERVO2 = 1000;

const int buttonGetBluePen = 32;
const int buttonGetRedPen = 33;
bool lastState_buttonGetBluePen = false;
bool lastState_buttonGetRedPen = false;

void IRAM_ATTR doCounter();
void updateDisplay();
void calculateAmount();
void purchesPen(String PenColor);
void sendLineNotify(String message, String imageUrl = "");

const int countdownTime = 30;
unsigned long previousMillis = 0;
bool timerIsActive = false;
void setup()
{
  Serial.begin(115200);
  lcd.begin(16, 2);
  lcd.backlight();

  lcd.setCursor(0, 0);
  lcd.print("System Starting.");
  delay(100);
  lcd.clear();

  lcd.setCursor(1, 0);
  lcd.print("Please Connect");
  lcd.setCursor(5, 1);
  lcd.print("WiFi...");
  delay(100);
  lcd.clear();

  WiFiManager wm;
  bool res;

  IPAddress staticIP(192, 168, 4, 1);
  IPAddress gateway(192, 168, 4, 1);
  IPAddress subnet(255, 255, 255, 0);
  wm.setAPStaticIPConfig(staticIP, gateway, subnet);

  lcd.setCursor(6, 0);
  lcd.print("WiFi");
  lcd.setCursor(2, 1);
  lcd.print("Connecting..");
  delay(1000);

  wm.setTimeout(30000);

  if (!wm.autoConnect(WIFI_NAME, WIFI_PASSWORD))
  {
    Serial.println("Failed to connect");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("AP Mode Active");
    lcd.setCursor(0, 1);
    lcd.print("Connect to WiFi");

    previousMillis = millis();
    timerIsActive = true;
  }
  else
  {
    // sendLineNotify("เชื่อมต่อกับเครื่องขายปากกาสำเร็จแล้ว! ✅", "https://lh3.googleusercontent.com/proxy/PuikqzwSBdertvKVZSgkEWbpwbt8eB-fZHFc8RXMAozAyvFRFLvS3BVBwzzcYqcNMYo_pGA2PmsBEe0yHYK8ykXZB_1d1Jwi7Le7TYhZn5b-D3mXud9DVVUJ_IIsiEE");
    Serial.println("เชื่อมต่อสำเร็จแล้ว!");
    lcd.clear();
    lcd.setCursor(6, 0);
    lcd.print("WiFi");
    lcd.setCursor(3, 1);
    lcd.print("Connected!");
    delay(2000);
  }
  pinMode(coinValidatorPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(coinValidatorPin), doCounter, FALLING);
  pinMode(buttonGetBluePen, INPUT_PULLUP);
  pinMode(buttonGetRedPen, INPUT_PULLUP);

  servo1.attach(servoPin1);
  servo2.attach(servoPin2);

  servo1.write(LEFT);
  servo2.write(LEFT);
  delay(150);
  servo1.write(RIGHT);
  servo2.write(RIGHT);
  delay(150);
  servo1.write(STOP);
  servo2.write(STOP);

  updateDisplay();
}

void loop()
{
  if (timerIsActive)
  {
    unsigned long currentMillis = millis();
    unsigned long elapsedMillis = currentMillis - previousMillis;
    int remainingTime = countdownTime - (elapsedMillis / 1000);

    lcd.setCursor(0, 0);
    lcd.print("AP Mode Active");
    lcd.setCursor(0, 1);
    lcd.print("Time left: " + String(remainingTime) + " s");

    if (remainingTime <= 0)
    {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Restarting...");
      delay(2000);
      ESP.restart();
    }
  }

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

  if (currentBluePen != 0)
  {
    if (digitalRead(buttonGetBluePen) == LOW && !lastState_buttonGetBluePen)
    {
      lastState_buttonGetBluePen = true;
      purchesPen("blue");
    }
    if (digitalRead(buttonGetRedPen) == LOW && !lastState_buttonGetRedPen)
    {
      lastState_buttonGetRedPen = true;
      purchesPen("red");
    }
  }

  if (currentRedPen != 0)
  {
    if (digitalRead(buttonGetBluePen) == HIGH)
    {
      lastState_buttonGetBluePen = false;
    }
    if (digitalRead(buttonGetRedPen) == HIGH)
    {
      lastState_buttonGetRedPen = false;
    }
  }
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
    else if (pulseCount >= 3 && pulseCount < 6)
    {
      totalAmount += 5;
      Serial.println("5 บาท");
    }
    else if (pulseCount > 5)
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
void purchesPen(String PenColor)
{
  if (pens >= 1)
  {
    Serial.print("purches Pen!");

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("You have success");
    lcd.setCursor(0, 1);
    lcd.print("Purchased 1 pen!");
    delay(100);

    pens -= 1;
    totalAmount -= pricePen;

    if (PenColor == "blue")
    {
      bluePenPurchased += 1;
      currentBluePen -= 1;
      if (currentBluePen <= 5)
      {
        sendLineNotify("\n🔵 ปากกาสีน้ำเงินใกล้จะหมดแล้ว\n\nจำนวนที่เหลือ: " + String(currentBluePen) + " ด้าม");
      }
      if (currentBluePen == 0)
      {
        sendLineNotify("\n🔵 ปากกาสีน้ำเงินหมดแล้ว\n\nกรุณาเติมปากกา!!");
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Can't buy a Pen");
        lcd.setCursor(0, 1);
        lcd.print("Please refill it");
        delay(2000);
      }
    }
    else if (PenColor == "red")
    {
      redPenPurchased += 1;
      currentRedPen -= 1;
      if (currentRedPen <= 5)
      {
        sendLineNotify("\n🔴 ปากกาสีแดงใกล้จะหมดแล้ว\n\nจำนวนที่เหลือ: " + String(currentRedPen) + " ด้าม");
      }
      if (currentRedPen == 0)
      {
        sendLineNotify("\n🔴 ปากกาสีแดงหมดแล้ว\n\nกรุณาเติมปากกา!!");
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Can't buy a Pen");
        lcd.setCursor(0, 1);
        lcd.print("Please refill it");
        delay(2000);
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