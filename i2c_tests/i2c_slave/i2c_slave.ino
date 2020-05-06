//code for arduino
#include <Wire.h>

const int VEC_MAX = 60;
float vec[VEC_MAX] = { 1.11333, 2.11, 3.11, 4.11, 5.11, 6.11, 7.11, 8.11, 9.11, 10.11, 11.11, 12.11, 13.11, 14.11, 15.11, 16.11, 17.11, 18.11, 19.11, 20.11,
                       1.22, 2.22, 3.22, 4.22, 5.22, 6.22, 7.22, 8.22, 9.22, 10.22, 11.22, 12.22, 13.22, 14.22, 15.22, 16.22, 17.22, 18.22, 19.22, 20.22,
                       1.33, 2.33, 3.33, 4.33, 5.33, 6.33, 7.33, 8.33, 9.33, 10.33, 11.33, 12.33, 13.33, 14.33, 15.33, 16.33, 17.33, 18.33, 19.33, 20.33
                     };
float split_vec[12][5];
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


int counter = 0;
void receiveEvent(int bytes) { 
  counter = 0; 
} 

void requestEvent() {
  Serial.println(counter);
  if (counter == 0) {
    for (int i = 0; i < VEC_MAX; i++) {
      split_vec[i / 5][i % 5] = vec[i];
    }
    counter ++;
    /*
      for (int i = 0; i < 12; i++) {
      for (int j = 0; j < 5; j++) {
         Serial.print(split_vec[i][j]);
         Serial.print(" ");
      }
      Serial.println();
      }
    */
    Wire.write((uint8_t*) split_vec[0], 20);
  }
  else if (counter < 12) {
    Wire.write((uint8_t*) split_vec[counter], 20);
    counter ++;
  }

  if (counter >= 12)
    counter = 0;
}



