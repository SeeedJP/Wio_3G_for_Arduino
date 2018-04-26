#include <Wio3GforArduino.h>

#define BUTTON_PIN  (WIO_D38)
#define INTERVAL    (100)

void setup() {
  SerialUSB.begin(115200);
  pinMode(BUTTON_PIN, INPUT);
}

void loop() {
  int buttonState = digitalRead(BUTTON_PIN);
  SerialUSB.print(buttonState ? '*' : '.');
  
  delay(INTERVAL);
}

