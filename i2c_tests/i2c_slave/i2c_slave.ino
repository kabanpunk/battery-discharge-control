//code for arduino
#include <Wire.h>  
const int VEC_MAX = 60;
float vec[VEC_MAX] = { 3.83, 3.83, 3.83, 3.83, 3.83, 3.83, 3.83, 3.83, 3.83, 3.83, 3.76, 3.76, 3.76, 3.76, 3.76, 3.76, 3.76, 3.76, 3.76, 3.76,
                       0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 21.8, 21.8, 21.8, 21.8, 21.8, 21.8, 21.8, 21.8, 21.8, 21.8,
                       0.167, 0.168, 0.168, 0.169, 0.169, 0.17, 0.17, 0.171, 0.171, 0.172, 0.172, 0.173, 0.173, 0.174, 0.174, 0.175, 0.175, 0.176, 0.176, 0.177
                     };
float split_vec[12][5];
float V = 3.85;
float A = 22.5;
float R = 4.56;

void setup() {
  Wire.begin(8);
  Serial.begin(9600); 
  Wire.onReceive(receiveEvent);
  Wire.onRequest(requestEvent);
  Serial.println("START ARDUINO");
}


void loop() {
  delay(100);
}


int counter = -1;
void receiveEvent(int bytes) { 
  counter = -1; 
} 

void requestEvent() {
  Serial.println(counter);
  if (counter == -1) {
     float fvec[3] = {V, A, R};
     Wire.write((uint8_t*) fvec, sizeof(fvec));
     counter = 0;
  }
  else if (counter == 0) {
    for (int i = 0; i < VEC_MAX; i++) {
      split_vec[i / 5][i % 5] = vec[i];
    }
    counter ++; 
    Wire.write((uint8_t*) split_vec[0], 20);
  }
  else if (counter < 12) {
    Wire.write((uint8_t*) split_vec[counter], 20);
    counter ++;
  }

  if (counter >= 12)
    counter = -1;
}



