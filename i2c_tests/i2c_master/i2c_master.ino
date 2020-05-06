// code for ESP8266
#include <Wire.h>

float VOLTAGES[20];
float AMPERAGES[20];
float TIMES[20];

void setup() {
  Wire.begin(D1, D2);
  Serial.begin(9600);
  Serial.println("START");
  smart_read(8) ; //30 ms

}

void loop() {

}

float smart_read(int address) { 
  Wire.beginTransmission(address);
  Wire.write(0);
  Wire.endTransmission();
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 4; j++) {
      float vec[5];
      uint8_t* vp = (uint8_t*) vec;

      Wire.requestFrom(8, sizeof(vec));
      while (Wire.available()) {
        *vp++ = Wire.read();
      }

      for (int t = 0; t < 5; t++) {
        if (i == 0)
          VOLTAGES[j * 5 + t] = vec[t];
        else if (i == 1)
          AMPERAGES[j * 5 + t] = vec[t];
        else if (i == 2)
          TIMES[j * 5 + t] = vec[t];
      }
    }
  } 
}
