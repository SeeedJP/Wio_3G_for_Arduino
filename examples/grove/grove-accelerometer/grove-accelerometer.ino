#include <Wio3GforArduino.h>

#define INTERVAL    (100)

#define I2C_ADDRESS   (0x53)
#define REG_POWER_CTL (0x2d)
#define REG_DATAX0    (0x32)

Wio3G Wio;

void setup() {
  delay(200);
  
  SerialUSB.begin(115200);
  SerialUSB.println("");
  SerialUSB.println("--- START ---------------------------------------------------");

  SerialUSB.println("### I/O Initialize.");
  Wio.Init();
  
  SerialUSB.println("### Power supply ON.");
  Wio.PowerSupplyGrove(true);
  delay(500);

  SerialUSB.println("### Sensor Initialize.");
  AccelInitialize();

  SerialUSB.println("### Setup completed.");
}

void loop() {
  float x;
  float y;
  float z;
  AccelReadXYZ(&x, &y, &z);
  SerialUSB.print(x);
  SerialUSB.print(' ');
  SerialUSB.print(y);
  SerialUSB.print(' ');
  SerialUSB.println(z);

  delay(INTERVAL);
}

////////////////////////////////////////////////////////////////////////////////////////
//

void AccelInitialize() {
  WireI2C.begin();
  WireI2C.beginTransmission(I2C_ADDRESS);
  WireI2C.write(REG_POWER_CTL);
  WireI2C.write(0x08);
  WireI2C.endTransmission();
}

void AccelReadXYZ(float* x, float* y, float* z) {
  WireI2C.beginTransmission(I2C_ADDRESS);
  WireI2C.write(REG_DATAX0);
  WireI2C.endTransmission();

  if (WireI2C.requestFrom(I2C_ADDRESS, 6) != 6) {
    *x = 0;
    *y = 0;
    *z = 0;
    return;
  }

  int16_t val;
  ((uint8_t*)&val)[0] = WireI2C.read();
  ((uint8_t*)&val)[1] = WireI2C.read();
  *x = val * 2.0 / 512;
  ((uint8_t*)&val)[0] = WireI2C.read();
  ((uint8_t*)&val)[1] = WireI2C.read();
  *y = val * 2.0 / 512;
  ((uint8_t*)&val)[0] = WireI2C.read();
  ((uint8_t*)&val)[1] = WireI2C.read();
  *z = val * 2.0 / 512;
}

////////////////////////////////////////////////////////////////////////////////////////

