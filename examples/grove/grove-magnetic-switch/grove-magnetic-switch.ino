#include <Wio3GforArduino.h>

#define MAGNETIC_SWITCH_PIN (WIO_D38)
#define INTERVAL            (100)

void setup() {
  SerialUSB.begin(115200);
  pinMode(MAGNETIC_SWITCH_PIN, INPUT);
}

void loop() {
  int switchState = digitalRead(MAGNETIC_SWITCH_PIN);
  SerialUSB.print(switchState ? '*' : '.');
  
  delay(INTERVAL);
}

