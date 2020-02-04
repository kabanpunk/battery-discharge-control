#include <Arduino.h>
#include <TM1637Display.h>

// Module connection pins (Digital Pins)
#define CLK 12
#define DIO 11

// The amount of time (in milliseconds) between tests
#define TEST_DELAY   2000

const uint8_t A = SEG_E | SEG_F | SEG_A | SEG_B | SEG_C | SEG_G;
const uint8_t U = SEG_E | SEG_F | SEG_B | SEG_C | SEG_D;
const uint8_t t = SEG_E | SEG_F | SEG_G | SEG_D;

TM1637Display display(CLK, DIO);

float sr(float f, int n) {
  return round(f * pow(10, n)) / pow(10, n);
}
void printFloat(float f, uint8_t C) {
  uint8_t data[] = { 0x00 , 0x00, 0x00, C };
  String s = String(f);

  int di = s.indexOf('.');
  String ns = String(sr(f, 4 - di));
  ns.remove(ns.indexOf('.'), 1);
  int N = ns.length();
  if (N > 0) {
    data[2] = display.encodeDigit(int(ns[2]));
  }
  if (N > 1) {
    data[1] = display.encodeDigit(int(ns[1]));
  }
  if (N > 2) {
    data[0] = display.encodeDigit(int(ns[0]));
  }
  if (String(int(f)).length()<3)
    data[di - 1] += 0x80;
  display.setSegments(data);
}

void setup()
{
  Serial.begin(9600);
  display.setBrightness(0x0f);
  for (int i = 9900; i < 100000; i++) {
    float f = i / 100.;
    printFloat(f);
    Serial.println(f);
    delay(10);
  }

}

void loop()
{
}
