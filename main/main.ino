#include <Arduino.h>
#include <TM1637Display.h>
#include <Wire.h>
#include <Adafruit_ADS1015.h>

Adafruit_ADS1115 ads;

// Module connection pins (Digital Pins)
#define CLK 12
#define DIO 11

// The amount of time (in milliseconds) between tests
#define TEST_DELAY   2000

const uint8_t A = SEG_E | SEG_F | SEG_A | SEG_B | SEG_C | SEG_G;
const uint8_t V = SEG_E | SEG_F | SEG_B | SEG_C | SEG_D;
const uint8_t t = SEG_E | SEG_F | SEG_G | SEG_D;

TM1637Display display(CLK, DIO);

float sr(float f, int n) {
  return round(f * pow(10, n)) / pow(10, n);
}

String delzeros(String s) {
  for (int i = s.length() - 1; i > 0; i--) {
    if (s[i] == '.') break;
    if (s[i] == '0') s.remove(i);
  }
  return s;
}

void printFloat(float f, uint8_t C = 0x00) {
  f = abs(f);
  uint8_t data[] = { 0x00 , 0x00, 0x00, C };
  String s = String(f);
  int tip = String(int(f)).length();
  int di = s.indexOf('.');

  if (C == 0x00) {
    String ns = delzeros(String(sr(f, 4 - tip)));
    //Serial.println(ns);
    ns.remove(ns.indexOf('.'), 1);
    int N = ns.length();
    int c = 0;
    if (N > 0) {
      data[3] = display.encodeDigit(int(ns[N - 1])); c++;
    }
    if (N > 1) {
      data[2] = display.encodeDigit(int(ns[N - 2])); c++;
    }
    if (N > 2) {
      data[1] = display.encodeDigit(int(ns[N - 3])); c++;
    }
    if (N > 3) {
      data[0] = display.encodeDigit(int(ns[N - 4])); c++;
    }
    //Serial.println("di: " + String(di) + "; c: " + String(c));
    if (di > 0 and int(f) != f)
      data[ di + 3 - c] += 0x80;
    display.setSegments(data);
  }
  else {
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
    if (di > 0 and int(f) != f)
      data[ di - 1 ] += 0x80;
    display.setSegments(data);
  }
}

void setup()
{
  Serial.begin(9600);
  display.setBrightness(0x0f);

  pinMode(2, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(2), btn, FALLING );

  //0.5HZ
  cli();
  TCCR1A = 0;// set entire TCCR1A register to 0
  TCCR1B = 0;// same for TCCR1B
  TCNT1  = 0;//initialize counter value to 0
  // set compare match register for 1hz increments
  OCR1A = 31249;// = (16*10^6) / (COUNT_HZ *1024) - 1 (must be <65536)
  // turn on CTC mode
  TCCR1B |= (1 << WGM12);
  // Set CS12 and CS10 bits for 1024 prescaler
  TCCR1B |= (1 << CS12) | (1 << CS10);
  // enable timer compare interrupt
  TIMSK1 |= (1 << OCIE1A);
  sei();

  ads.begin();
}

float amperage = 0;
float voltage = 0; 
float minutes = 0;
int toggle = 0;
float endTime = 0;
bool activeToggle = false;

void loop()
{
  voltage = abs(ads.readADC_Differential_2_3() * 0.1875F / 1000.);
  amperage = abs(ads.readADC_Differential_0_1() * 0.015625F / 2.);
 
  if (activeToggle and voltage < 2.6) {
    toggle = -1;
    printFloat(endTime);
    digitalWrite(5, LOW); 
    return;
  }
  endTime = ( millis() - minutes ) / 60000.;
  //Serial.println("amperage: " + String(amperage) + "; voltage: " + String(voltage));
}

ISR(TIMER1_COMPA_vect) {
  if (toggle == 0) {
    printFloat(amperage, A);
    toggle++;
  }
  else if (toggle == 1) {
    printFloat(voltage, V);
    toggle++;
  }
  else if (toggle == 2) {
    if (minutes != 0) {
      printFloat(( millis() - minutes ) / 60000.);
    }
    toggle = 0;
  }
}

bool falling = true;
int countClicks = 0;
void btn() {
  static unsigned long last_interrupt_time = 0;
  unsigned long interrupt_time = millis();
  if (interrupt_time - last_interrupt_time > 25)
  {
    if (falling) { 
      falling = false;
      if (countClicks == 0) {
        minutes = millis();
        digitalWrite(5, HIGH);
        toggle = 0;
        activeToggle = true;
        countClicks++;
      }
      else if (countClicks == 1) {
        minutes = 0;
        toggle = 0;
        endTime = 0;
        activeToggle = false;
        countClicks = 0;
      }
    }
    else {
      falling = true;
    }
  }
  last_interrupt_time = interrupt_time;
}
