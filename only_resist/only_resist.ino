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
const uint8_t R = SEG_E | SEG_G;
const uint8_t t = SEG_E | SEG_F | SEG_G | SEG_D;

TM1637Display display(CLK, DIO);

float VOLTAGE_THRESHOLD = 2.6;
float CAPACIRY_THRESHOLD = 0.05;
float amperage = 0;
float voltage = 0;
float resistance = 0;
float capaciry = 0;
bool toggle = false;
long work_time = 0;
bool end_of_cycle_check = false;
long cycle_start_time = 0;
float cycle_end_time = 0;
bool dump_existence = false;

void update_ads() {
  amperage = abs(ads.readADC_Differential_0_1() * 0.015625F / 2.);
  voltage = abs(ads.readADC_Differential_2_3() * 0.1875F / 1000.);
  //amperage = (amperage < 0.5) ? 0. : amperage;
  voltage = (voltage < 0.5) ? 0. : voltage;
  capaciry = (amperage * millis()) / 3600000.;
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

void loop()
{
  update_ads();

  if (!dump_existence and capaciry > CAPACIRY_THRESHOLD) {
    float sum_a = 0, sum_v = 0;
    int K = 10;
    Serial.println("\n\n");
    for (int i = 0; i < K; i++) {
      Serial.println("Напряжение: " + String(voltage, 6) + "; Сила тока: " + String(amperage, 6)  );
      sum_a += amperage;
      sum_v += voltage;
      update_ads();
    }
    Serial.println("Среднее напряжение: " + String(sum_v / K, 6) + "; Средняя сила тока: " + String(sum_a / K, 6));
    Serial.println("----------------------------------");

    digitalWrite(5, LOW);
    sum_a = 0, sum_v = 0;
    for (int i = 0; i < K; i++) {
      Serial.println("Напряжение: " + String(voltage, 6) + "; Сила тока: " + String(amperage, 6)  );
      sum_a += amperage;
      sum_v += voltage;
      update_ads();
    }
    Serial.println("Среднее напряжение: " + String(sum_v / K, 6) + "; Средняя сила тока: " + String(sum_a / K, 6));
    Serial.println("----------------------------------");
    float middle_v1 = sum_v / K;

    digitalWrite(5, HIGH);
    sum_a = 0, sum_v = 0;
    for (int i = 0; i < K; i++) {
      Serial.println("Напряжение: " + String(voltage, 6) + "; Сила тока: " + String(amperage, 6)  );
      sum_a += amperage;
      sum_v += voltage;
      update_ads();
    }
    Serial.println("Среднее напряжение: " + String(sum_v / K, 6) + "; Средняя сила тока: " + String(sum_a / K, 6));
    Serial.println("----------------------------------");
    resistance = (middle_v1 - (sum_v / K)) * 1000; //милливольт
    Serial.println("Сопротивление: " + String(resistance, 6));
    Serial.println("\n\n");

    dump_existence = true;
  }


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
  if (mode < 0) {
    if (screen_counter == 0) {
      screen_counter ++;
      display.printFloat(cycle_end_time, 4);
    }
    else if (screen_counter == 1) { 
      display.printFloat(resistance, 3, R); 
      screen_counter = 0;
    }
  }
  else {
    if (screen_counter == 0) {
      screen_counter ++;
      display.printFloat(voltage, 3, V);
    }
    else if (screen_counter == 1) {
      screen_counter ++;
      display.printFloat(amperage, 3, A);
      if (mode == 0)
        screen_counter = 0;
    }
    else {
      screen_counter = 0;
      display.printFloat((millis() - cycle_start_time) / 60000., 4);
    }
  }
}

ISR(TIMER1_COMPA_vect) {
  if (end_of_cycle_check) {
    Serial.println("Конец цикла");
    //Serial.println("Время: " + String(cycle_end_time));
    flipping_screens(-1);
  }
  else if (toggle) {
    Serial.println("Режим разрядки");
    Serial.println("Напряжение: " + String(voltage, 6)  + "; Сила тока: " + String(amperage, 6) + "; Ёмкость: " + String(capaciry, 6)  + "; Время: " + String((millis() - cycle_start_time) / 60000.));
    flipping_screens(1);
  }
  else {
    Serial.println("Режим ожидания");
    Serial.println("Напряжение: " + String(voltage, 6)  + "; Сила тока: " + String(amperage, 6));
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
