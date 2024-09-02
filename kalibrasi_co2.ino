#include "MQ135.h"
void setup () {
  Serial.begin (9600);
}
void loop() {
//  MQ135 gasSensor1 = MQ135(A0); // Attach sensor to pin A0
  MQ135 gasSensor2 = MQ135(A13); // Attach sensor to pin A0
  MQ135 gasSensor3 = MQ135(A14); // Attach sensor to pin A0
//  float rzero1 = gasSensor1.getRZero();
  float rzero2 = gasSensor2.getRZero();
  float rzero3 = gasSensor3.getRZero();
//  Serial.print (rzero2);
//  Serial.print("\t");
  Serial.println (rzero2);
  Serial.print("\t");
  Serial.println (rzero3);

  delay(1000);
}
