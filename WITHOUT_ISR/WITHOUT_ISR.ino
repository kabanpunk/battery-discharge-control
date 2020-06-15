#define THRESHOLD_VOLTAGE 3
#define THRESHOLD_CAPACIRY 1
 
#include <Arduino.h>
#include <TM1637Display.h>
#include <Wire.h>
#include <Adafruit_ADS1015.h>

Adafruit_ADS1115 ads;

#define CLK 12
#define DIO 11

TM1637Display display(CLK, DIO);

const uint8_t A = SEG_E | SEG_F | SEG_A | SEG_B | SEG_C | SEG_G;
const uint8_t V = SEG_E | SEG_F | SEG_B | SEG_C | SEG_D;
const uint8_t R = SEG_E | SEG_G;

float amperage = 0, voltage = 0, capaciry = 0, resistance = 0, operationTime = 0, startTime = 0, stopTime = 0;

long timerMillis = 0, screenCounter = 0; // при смене режима (currentMode) screenCounter должен приравниваться к нулю

int currentMode = 0; // 0 - ожидание, 1 - разрядка, 2 - конец цикла ( 0 -> 1 -> 2 -> 0)

bool buttonLastState = 0, dumpPresence = 0;

void update() {
  long ms = millis() - startTime;
  amperage = abs(ads.readADC_Differential_0_1() * 0.015625F / 2.);
  voltage  = abs(ads.readADC_Differential_2_3() * 0.1875F / 1000.);
  amperage = (amperage < 0.5) ? 0. : amperage;
  voltage  = (voltage  < 0.5) ? 0. : voltage;
  capaciry = (amperage * ms) / 3600000.;
  operationTime = ms / 60000.;
}

void setup()
{
  Serial.begin(9600);

  display.setBrightness(0x0f);

  ads.begin();

  Serial.println("Ожидание начала цикла");

}

void timer() {
  if (currentMode == 0) {
    if (screenCounter == 0) {
      display.printFloat(voltage, 3, V); screenCounter ++;
    }
    else if (screenCounter == 1) {
      display.printFloat(amperage, 3, A); screenCounter = 0;
    }
  }
  else if (currentMode == 1) {
    if (screenCounter == 0) {
      display.printFloat(voltage, 3, V); screenCounter ++;
    }
    else if (screenCounter == 1) {
      display.printFloat(amperage, 3, A); screenCounter ++;
    }
    else if (screenCounter == 2) {
      display.printFloat(operationTime, 4); screenCounter = 0;
    }
  }
  else if (currentMode == 2) {
    if (screenCounter == 0) {
      display.printFloat(stopTime, 4); screenCounter ++;
    }
    else if (screenCounter == 1) {
      display.printFloat(resistance, 3, R); screenCounter = 0;
    }
  }
}

void loop()
{
  update(); 

  //Serial.println("Напряжение: " + String(voltage, 6)  + "; Сила тока: " + String(amperage, 6) + "; Ёмкость: " + String(capaciry, 6)  + "; Время: " + String(operationTime, 6) + "; Режим: " + String(currentMode, 6));
 
  if (currentMode == 1) {  
    if (voltage < THRESHOLD_VOLTAGE) {
      digitalWrite(5, LOW);
      stopTime = operationTime;
      currentMode = 2;
      screenCounter = 0; 
    }
    if (!dumpPresence and capaciry > THRESHOLD_CAPACIRY) {
      Serial.println("capaciry: "+String(capaciry,6)  + "; Время: " + String(operationTime, 6));
      dumpPresence = 1;
      float sum_a = 0, sum_v = 0;
      int K = 20;
      for (int i = 0; i < K; i++) {
        sum_a += amperage;
        sum_v += voltage;
        update();
      }
      digitalWrite(5, LOW);
      sum_a = 0, sum_v = 0;
      for (int i = 0; i < K; i++) {
        sum_a += amperage;
        sum_v += voltage;
        update();
      }
      float middle_v1 = sum_v / K;
      digitalWrite(5, HIGH);
      sum_a = 0, sum_v = 0;
      for (int i = 0; i < K; i++) {
        sum_a += amperage;
        sum_v += voltage;
        update();
      }
      resistance = (middle_v1 - (sum_v / K)) / (sum_a / K) * 1000.; //милливольт
      Serial.println("Сопротивление: " + String(resistance, 6)); 
    }
  }

  bool buttonState = !digitalRead(2);
  if (buttonState and buttonLastState != buttonState and currentMode != 1) {
    Serial.println("BUTTON"); 
    if (currentMode == 0) {
      Serial.println("Включение режима разрядки");
      currentMode = 1;
      dumpPresence = 0;
      digitalWrite(5, HIGH);
      startTime = millis();
    }
    else if (currentMode == 2) {
      Serial.println("Включение режима ожидания");
      currentMode = 0;
    } 
    screenCounter = 0;
  }
  buttonLastState = buttonState;

  if (millis() - timerMillis > 2000) {
    timer();
    timerMillis = millis();
  }
}  
