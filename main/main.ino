float VOLTAGE_THRESHOLD = 2.6;  

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
  return round(f * pow(10, n)) / pow(10, n) / 1.;
}

String remove_zeros(String s, int l) { 
  if (s[s.length() - 1] == '0' and s[s.length() - 2] == '0'){ //((s.indexOf('.') !=  s.length() - 2 ? s.length() : s.length()  - 1) <= l) { 
      s.remove(s.length() - 1); 
    return s;
  } 
  int i = s.length() - 1;
  while (i > 0 and (s[i] == '0' or s[i] == '.')) { 
    if (s[i] == '.') {
      s.remove(i);
      break;
    }
    s.remove(i);
    i -= 1;
  }
  return s;
}

void printFloat(float x, int l, uint8_t C = 0x00) {
  uint8_t data[] = { 0x00 , 0x00, 0x00, C };
  String s = String( sr( x, l - String(int(x)).length() ) );

  s = remove_zeros(s, l); 
  int i = l - 1, j = s.length() - 1;
  while (i >= 0 and j >= 0) {
    if (s[j] == '.') {
      data[i] += 0x80;
      j -= 1;
    }
    data[i] += display.encodeDigit(int(s[j]));
    i -= 1;
    j -= 1;
  }
  display.setSegments(data);
}

void setup()
{
  Serial.begin(9600);
  display.setBrightness(0x0f); 
  pinMode(2, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(2), btn, FALLING );

  //0.5HZ
  cli();
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1  = 0;
  OCR1A = 15624;//31249;// = (16*10^6) / (COUNT_HZ *1024) - 1 (must be <65536)
  // turn on CTC mode
  TCCR1B |= (1 << WGM12);
  // Set CS12 and CS10 bits for 1024 prescaler
  TCCR1B |= (1 << CS12) | (1 << CS10);
  // enable timer compare interrupt
  TIMSK1 |= (1 << OCIE1A);
  sei();

  ads.begin();
  Serial.println("Ожидание начала цикла");
}

float amperage = 0;
float voltage = 0;
bool toggle = false;
long work_time = 0;
bool end_of_cycle_check = false;
long cycle_start_time = 0;
float cycle_end_time = 0;

void loop()
{
  voltage = abs(ads.readADC_Differential_2_3() * 0.1875F / 1000.);
  amperage = abs(ads.readADC_Differential_0_1() * 0.015625F / 2.);

  if (toggle and !end_of_cycle_check and voltage < VOLTAGE_THRESHOLD) {
    cycle_end_time = (millis() - cycle_start_time) / 60000.;
    toggle = false;
    end_of_cycle_check = true;
  }
  if (toggle) {
    digitalWrite(5, HIGH);
  }
  else {
    digitalWrite(5, LOW);
  }
}

int screen_counter = 0;
void flipping_screens(int mode = 0) {
  if (screen_counter == 0) {
    screen_counter ++;
    printFloat(voltage, 3, V);
  }
  else if (screen_counter == 1) {
    screen_counter ++;
    printFloat(amperage, 3, A);
    if (mode == 0)
      screen_counter = 0;
  }
  else {
    screen_counter = 0;
    printFloat((millis() - cycle_start_time) / 60000., 4);
  }
}

ISR(TIMER1_COMPA_vect) { 
  if (end_of_cycle_check) {
    Serial.println("Конец цикла");
    Serial.println("Время: " + String(cycle_end_time));
    printFloat(cycle_end_time, 4);
  }
  else if (toggle) {
    Serial.println("Режим разрядки");
    Serial.println("Напряжение: " + String(voltage) + "; Сила тока: " + amperage + "; Время: " + String((millis() - cycle_start_time) / 60000.));
    flipping_screens(1);
  }
  else {
    Serial.println("Режим ожидания");
    Serial.println("Напряжение: " + String(voltage) + "; Сила тока: " + amperage);
    flipping_screens(0);
  }
  Serial.println("------------------\n"); 
}

void btn() {
  static unsigned long last_interrupt_time = 0;
  unsigned long interrupt_time = millis();
  if (interrupt_time - last_interrupt_time > 50)
  {
    if (end_of_cycle_check) {
      end_of_cycle_check = false;
      toggle = false;
      Serial.println("Ожидание начала цикла");
    }
    else if (amperage < 1) {
      if (toggle) {
        toggle = false;
        Serial.println("Режим сменён на " + String(toggle) + " (0 - батарея не разрежается; 1 - разрежается)");
      }
      else {
        end_of_cycle_check = false;
        cycle_start_time = millis();
        toggle = true;
        Serial.println("Режим сменён на " + String(toggle) + " (0 - батарея не разрежается; 1 - разрежается)");
      }
    }
  }
  last_interrupt_time = interrupt_time;
}
