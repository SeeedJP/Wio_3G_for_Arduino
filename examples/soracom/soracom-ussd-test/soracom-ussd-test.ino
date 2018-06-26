#include <Wio3GforArduino.h>

Wio3G Wio;


void setup() {
  delay(200);

  SerialUSB.begin(115200);
  SerialUSB.println("");
  SerialUSB.println("--- START ---------------------------------------------------");

  SerialUSB.println("### I/O Initialize.");
  Wio.Init();

  SerialUSB.println("### Power supply ON.");
  Wio.PowerSupplyCellular(true);
  delay(500);

  SerialUSB.println("### Turn on or reset.");
  if (!Wio.TurnOnOrReset()) {
    SerialUSB.println("### ERROR! ###");
    return;
  }

  SerialUSB.println("### Registering location.");
  if (!Wio.WaitForCSRegistration()) {
    SerialUSB.println("### ERROR! ###");
    return;
  }

  SerialUSB.println("### Sending USSD.");

  const char* message = "*901011*123#";  // Beam
  //const char* message = "*901021*123# ";  // Funnel
  //const char* message = "*901031*123#";  // Harvest
  char response[256];
  size_t respLen = sizeof(response);

  if (!Wio.SendUSSD(message, response, &respLen)) {
    SerialUSB.println("### ERROR! ###");
    return;
  }

  SerialUSB.print("### Received response: ");
  SerialUSB.println(response);
}

void loop() {

}
