#pragma once

#include "Wio3GConfig.h"

#define PINNAME_TO_PIN(port, pin) ((port - 'A') * 16 + pin)

#define WIO_D38				PINNAME_TO_PIN('C', 6)
#define WIO_D39				PINNAME_TO_PIN('C', 7)
#define WIO_D20				PINNAME_TO_PIN('B', 4)
#define WIO_D19				PINNAME_TO_PIN('B', 3)
#define WIO_A6				PINNAME_TO_PIN('A', 6)
#define WIO_A7				PINNAME_TO_PIN('A', 7)
#define WIO_A4				PINNAME_TO_PIN('A', 4)
#define WIO_A5				PINNAME_TO_PIN('A', 5)
#define WIO_UART_D23		PINNAME_TO_PIN('B', 7)
#define WIO_UART_D22		PINNAME_TO_PIN('B', 6)
#define WIO_I2C_D24			PINNAME_TO_PIN('B', 8)
#define WIO_I2C_D25			PINNAME_TO_PIN('B', 9)

#define DEBUG_UART_CORE     (2)   // USART3
#define DEBUG_UART_TX_PIN   PINNAME_TO_PIN('D', 8)	// out
#define DEBUG_UART_RX_PIN   PINNAME_TO_PIN('D', 9)	// in

#define MODULE_UART_CORE    (1)   // USART2
#define MODULE_UART_TX_PIN  PINNAME_TO_PIN('A', 2)	// out
#define MODULE_UART_RX_PIN  PINNAME_TO_PIN('A', 3)	// in
// Power Supply
#define MODULE_PWR_PIN      PINNAME_TO_PIN('E', 9)  // out
// Turn On/Off
#define MODULE_PWRKEY_PIN   PINNAME_TO_PIN('B', 5)  // out
#define MODULE_RESET_PIN	PINNAME_TO_PIN('D', 5)  // out
// Status Indication
#define MODULE_STATUS_PIN   PINNAME_TO_PIN('B', 15) // in
// Main UART Interface
#define MODULE_CTS_PIN      PINNAME_TO_PIN('A', 1)  // in
#define MODULE_RTS_PIN      PINNAME_TO_PIN('A', 0)  // out
#define MODULE_DTR_PIN      PINNAME_TO_PIN('C', 5)  // out

#define LED_VDD_PIN			PINNAME_TO_PIN('E', 8)  // out
#define LED_PIN				PINNAME_TO_PIN('B', 1)  // out

#define GROVE_VCCB_PIN      PINNAME_TO_PIN('B', 10)	// out
#define GROVE_UART_CORE     (0)   // USART1
#define GROVE_UART_TX_PIN   WIO_UART_D22			// out
#define GROVE_UART_RX_PIN   WIO_UART_D23			// in
#define GROVE_I2C_CORE		(0)   // I2C1
#define GROVE_I2C_SCL_PIN	WIO_I2C_D25				// out
#define GROVE_I2C_SDA_PIN	WIO_I2C_D24				// in/out

extern HardwareSerial SerialUSB;
extern HardwareSerial SerialModule;
extern HardwareSerial SerialUART;
extern TwoWire WireI2C;
