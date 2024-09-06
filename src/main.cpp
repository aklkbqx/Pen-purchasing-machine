#define LINE_TOKEN "BAwPVl5Qq41LhakbJLOsgwCp8bidWP6tJzrf5rBgFzS"

#include <Arduino.h>
#include <WiFi.h>
#include <ESP32Servo.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WiFiManager.h>
#include <HTTPClient.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

// กำหนดชื่อและรหัสผ่านของ WIFI ที่ปล่อยออกมาจาก ESP32
const char *WIFI_NAME = "!PEN PURCHASING MACHINE";
const char *WIFI_PASSWORD = "11111111";

const int coinValidatorPin = 15;
const int speakerPin = 17;

Servo servo1;
Servo servo2;

const int servoPin1 = 12;
const int servoPin2 = 13;

const int LEFT = 0;
const int RIGHT = 180;
const int STOP = 90;
const int DELAY_SERVO1 = 1000;
const int DELAY_SERVO2 = 1000;

const int buttonGetBluePen = 32;
const int buttonGetRedPen = 33;
bool lastState_buttonGetBluePen = false;
bool lastState_buttonGetRedPen = false;

volatile int totalAmount = 0;
volatile unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 200;
volatile int pulseCount = 0;
volatile int pens = 0;
const unsigned long calculationDelay = 500;

const int pricePen = 7;

void IRAM_ATTR doCounter();
void updateDisplay();
void playSound(bool single);
void calculateAmount();
void purchesPen(String PenColor);
void sendLineNotify(String message);

void setup()
{
  Serial.begin(115200);
  lcd.begin();
  lcd.backlight();

  lcd.setCursor(0, 0);
  lcd.print("System Starting.");
  delay(2000);
  lcd.clear();

  lcd.setCursor(1, 0);
  lcd.print("Please Connect");
  lcd.setCursor(5, 1);
  lcd.print("WiFi...");
  delay(2000);
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

  if (!wm.autoConnect(WIFI_NAME, WIFI_PASSWORD))
  {
    Serial.println("Failed to connect");
    lcd.print("Failed to connect");
    delay(2000);
    ESP.restart();
  }
  else
  {
    sendLineNotify("เชื่อมต่อกับเครื่องขายปากกาสำเร็จแล้ว!");
    Serial.println("เชื่อมต่อสำเร็จแล้ว!");
    lcd.clear();
    delay(500);
    lcd.setCursor(6, 0);
    lcd.print("WiFi");
    lcd.setCursor(3, 1);
    lcd.print("Connected!");

    delay(4000);
    updateDisplay();
  }

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

  pinMode(coinValidatorPin, INPUT_PULLUP);
  pinMode(speakerPin, OUTPUT);

  pinMode(buttonGetBluePen, INPUT_PULLUP);
  pinMode(buttonGetRedPen, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(coinValidatorPin), doCounter, FALLING);
}

void loop()
{
  static unsigned long lastCalculationTime = 0;
  unsigned long currentTime = millis();

  if (pulseCount > 0 && (currentTime - lastDebounceTime) > calculationDelay)
  {
    calculateAmount();
    lastCalculationTime = currentTime;
  }

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
  if (digitalRead(buttonGetBluePen) == HIGH)
  {
    lastState_buttonGetBluePen = false;
  }
  if (digitalRead(buttonGetRedPen) == HIGH)
  {
    lastState_buttonGetRedPen = false;
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
    lcd.print("Total: ");
    lcd.print(totalAmount);
    lcd.print(" Bath.-");

    lcd.setCursor(0, 1);
    lcd.print("Pens: ");
    lcd.print(pens);
  }
}
void playSound(bool single)
{
  if (!single)
  {
    tone(speakerPin, 1000);
    delay(500);
    noTone(speakerPin);
    delay(500);
  }
  else
  {
    for (int i = 0; i < 3; i++)
    {
      tone(speakerPin, 1000);
      delay(100);
      noTone(speakerPin);
      delay(100);
    }
  }
}
void calculateAmount()
{
  if (pulseCount > 0)
  {
    Serial.print("จำนวน Pulse: ");
    Serial.println(pulseCount);

    if (pulseCount == 1)
      totalAmount += 1;
    else if (pulseCount == 2)
      totalAmount += 2;
    else if (pulseCount >= 3 && pulseCount < 5)
      totalAmount += 5;
    else if (pulseCount > 4)
      totalAmount += 10;
    else
      totalAmount = 0;

    Serial.print("เพิ่มเงิน: ");
    if (pulseCount == 1)
      Serial.println("1 บาท");
    else if (pulseCount == 2)
      Serial.println("2 บาท");
    else if (pulseCount >= 3 && pulseCount < 5)
      Serial.println("5 บาท");
    else if (pulseCount > 4)
      Serial.println("10 บาท");
    else
      Serial.println("0 บาท");

    if (totalAmount / pricePen > pens)
    {
      pens = totalAmount / pricePen;
      Serial.print("จำนวน pens: ");
      Serial.println(pens);
      playSound(true);
    }
    else
    {
      playSound(false);
    }

    pulseCount = 0;

    Serial.print("ยอดเงินรวมทั้งหมด: ");
    Serial.print(totalAmount);
    Serial.println(" บาท");

    updateDisplay();
  }
}
void purchesPen(String PenColor)
{
  if (pens >= 1)
  {
    Serial.print("purches Pen!");
    playSound(true);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("You have success");
    lcd.setCursor(0, 1);
    lcd.print("Purchased 1 pen!");
    delay(1000);

    pens -= 1;
    totalAmount -= pricePen;
    updateDisplay();

    if (PenColor == "red")
    {
      // moveServo(0, LEFT, 200);
      // moveServo(0, RIGHT, 200);
      // moveServo(0, STOP, 500);

      // moveServo(0, RIGHT, DELAY_SERVO1);
      // moveServo(0, STOP, 500);

      // moveServo(0, RIGHT, DELAY_SERVO1);
      // moveServo(0, STOP, 500);
    }
    else if (PenColor == "blue")
    {
      // moveServo(1, LEFT, 200);
      // moveServo(1, RIGHT, 200);
      // moveServo(1, STOP, 500);

      // moveServo(1, RIGHT, DELAY_SERVO2);
      // moveServo(1, STOP, 500);

      // moveServo(1, RIGHT, DELAY_SERVO2);
      // moveServo(1, STOP, 500);
    }
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
void sendLineNotify(String message)
{
  HTTPClient http;
  http.begin("https://notify-api.line.me/api/notify");
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  http.addHeader("Authorization", "Bearer " + String(LINE_TOKEN));

  String httpRequestData = "message=" + message;
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