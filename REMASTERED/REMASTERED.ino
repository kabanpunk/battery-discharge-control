#include <Arduino.h>
#include <TM1637Display.h>
#include <Wire.h>
#include <Adafruit_ADS1015.h>

Adafruit_ADS1115 ads;

double VOLTAGE_THRESHOLD = 2.6;  // ПОРОГ НАПРЯЖЕНИЯ
double CAPACITY_THRESHOLD  = 0.05; // ЁМКОСТЬ НА КОТОРОЙ ПРОВОДИТСЯ ЗАМЕР

// Module connection pins (Digital Pins)
#define CLK 12
#define DIO 11

// The amount of time (in milliseconds) between tests
#define TEST_DELAY   2000

const uint8_t A = SEG_E | SEG_F | SEG_A | SEG_B | SEG_C | SEG_G;
const uint8_t V = SEG_E | SEG_F | SEG_B | SEG_C | SEG_D;
const uint8_t t = SEG_E | SEG_F | SEG_G | SEG_D;
const uint8_t R = SEG_E | SEG_G;

TM1637Display display(CLK, DIO);

float sr(float f, int n) {
  return round(f * pow(10, n)) / pow(10, n) / 1.;
}

String remove_zeros(String s, int l) {
  if (s[s.length() - 1] == '0' and s[s.length() - 2] == '0') { //((s.indexOf('.') !=  s.length() - 2 ? s.length() : s.length()  - 1) <= l) {
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

String float_to_string(float x) {
  String s = String(round(x * 1000000));
  s = s.substring(0, String(x).indexOf('.')) + '.' + s.substring(String(x).indexOf('.'), s.length());
  return s;
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

  //Serial.println("Ожидание начала цикла");
}

float amperage = 0;
float voltage = 0;
float resistance = 0;
bool toggle = false;
double work_time = 0;
bool end_of_cycle_check = false;
long cycle_start_time = 0;
double cycle_end_time = 0; 
bool is_a_dump_recorded = false;
double BATTERY_CAPACITY = 0; 
 
void dump_record() {
  float sum_a = 0, sum_v = 0;
  int K = 30;
  //Serial.println("\n\n");
  for (int i = 0; i < K; i++) {
    //Serial.println("Напряжение: " + String(voltage, 6) + "; Сила тока: " + String(amperage, 6)  );
    sum_a += amperage;
    sum_v += voltage;
    read_ADC();
  }
  //Serial.println("Среднее напряжение: " + String(sum_v / K, 6) + "; Средняя сила тока: " + String(sum_a / K, 6));
  //Serial.println("----------------------------------");

  digitalWrite(5, LOW);
  delay(500);
  sum_a = 0, sum_v = 0;
  for (int i = 0; i < K; i++) {
    //Serial.println("Напряжение: " + String(voltage, 6) + "; Сила тока: " + String(amperage, 6)  );
    sum_a += amperage;
    sum_v += voltage;
    read_ADC();
  }
  //Serial.println("Среднее напряжение: " + String(sum_v / K, 6) + "; Средняя сила тока: " + String(sum_a / K, 6));
  //Serial.println("----------------------------------");
  float middle_v1 = sum_v / K;

  digitalWrite(5, HIGH);
  delay(500);
  sum_a = 0, sum_v = 0;
  for (int i = 0; i < K; i++) {
    //Serial.println("Напряжение: " + String(voltage, 6) + "; Сила тока: " + String(amperage, 6)  );
    sum_a += amperage;
    sum_v += voltage;
    read_ADC();
  }
  //Serial.println("Среднее напряжение: " + String(sum_v / K, 6) + "; Средняя сила тока: " + String(sum_a / K, 6));
  //Serial.println("----------------------------------");

  Serial.println("Среднее напряжение #1: " + String(middle_v1, 6) + "; Среднее напряжение #2: " + String(sum_v / K, 6) + "; Средняя сила тока: " + String(sum_a / K, 6));
  resistance = (middle_v1 - (sum_v / K)) / (sum_a / K) * 1000.; //миллиом
  Serial.println("Сопротивление: " + String(resistance, 6));
  Serial.println("\n\n");

  is_a_dump_recorded = true;
}

void read_ADC() {
  voltage = abs(ads.readADC_Differential_2_3() * 0.1875F / 1000.);
  amperage = abs(ads.readADC_Differential_0_1() * 0.015625F / 2.);
  amperage = amperage < 0.5 ? 0 : amperage;
  double BATTERY_CAPACITY_MS = amperage * (millis() - cycle_start_time);
  BATTERY_CAPACITY = BATTERY_CAPACITY_MS < 36000 ? 0 : BATTERY_CAPACITY_MS / 3600000.;
}

void loop()
{
  read_ADC();
  if (toggle and !end_of_cycle_check and voltage < VOLTAGE_THRESHOLD) {
    cycle_end_time = (millis() - cycle_start_time) / 60000.;
    toggle = false;
    end_of_cycle_check = true;
  }
  if (toggle) {
    if (!is_a_dump_recorded and CAPACITY_THRESHOLD > BATTERY_CAPACITY) { 
      dump_record();
    }
    else {
      digitalWrite(5, HIGH);
    }
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
  else if (mode == 1) {
    screen_counter = 0;
    printFloat((millis() - cycle_start_time) / 60000., 4);
  }
}

int sc_end = 0;
void flipping_screens_end() {
  if (sc_end == 0) {
    sc_end ++;
    display.printFloat(cycle_end_time, 4);
  }
  else if (sc_end == 1) {
    display.printFloat(resistance, 3, R);
    sc_end = 0;
  }
}

ISR(TIMER1_COMPA_vect) {
  if (end_of_cycle_check) {
    Serial.println("Конец цикла");
    Serial.println("Время: " + String(cycle_end_time));
    flipping_screens_end();
  }
  else if (toggle) {
    Serial.println("Режим разрядки");
    Serial.println("Напряжение: " + String(voltage) + "; Сила тока: " + amperage + "; Время: " + String((millis() - cycle_start_time) / 60000.));
    flipping_screens(1);
  }
  else {
    Serial.println("Режим ожидания");
    Serial.println("Напряжение: " + String(voltage) + "; Сила тока: ");
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
      else if (voltage > 1) {
        end_of_cycle_check = false;
        cycle_start_time = millis();
        toggle = true;
        is_a_dump_recorded = false;
        Serial.println("Режим сменён на " + String(toggle) + " (0 - батарея не разрежается; 1 - разрежается)");
      }
    }
  }
  last_interrupt_time = interrupt_time;
}
