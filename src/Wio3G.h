#pragma once

#include "Wio3GConfig.h"

#include "Internal/AtSerial.h"
#include "Internal/Wio3GSK6812.h"
#include <time.h>

#define WIO_TCP		(Wio3G::SOCKET_TCP)
#define WIO_UDP		(Wio3G::SOCKET_UDP)

class Wio3G
{
public:
	enum ErrorCodeType {
		E_OK = 0,
		E_UNKNOWN,
	};

	enum SocketType {
		SOCKET_TCP,
		SOCKET_UDP,
	};

private:
	SerialAPI _SerialAPI;
	AtSerial _AtSerial;
	Wio3GSK6812 _Led;
	ErrorCodeType _LastErrorCode;

private:
	bool ReturnOk(bool value)
	{
		_LastErrorCode = E_OK;
		return value;
	}
	int ReturnOk(int value)
	{
		_LastErrorCode = E_OK;
		return value;
	}
	bool ReturnError(int lineNumber, bool value, ErrorCodeType errorCode);
	int ReturnError(int lineNumber, int value, ErrorCodeType errorCode);

	bool IsBusy() const;
	bool IsRespond();
	bool Reset();
	bool TurnOn();

	bool HttpSetUrl(const char* url);

public:
	bool ReadResponseCallback(const char* response);	// Internal use only.

public:
	Wio3G();
	ErrorCodeType GetLastError() const;
	void Init();
	void PowerSupplyCellular(bool on);
	void PowerSupplyLed(bool on);
	void PowerSupplyGrove(bool on);
	void LedSetRGB(uint8_t red, uint8_t green, uint8_t blue);
	bool TurnOnOrReset();
	bool TurnOff();
	//bool Sleep();
	//bool Wakeup();

	int GetIMEI(char* imei, int imeiSize);
	int GetIMSI(char* imsi, int imsiSize);
	int GetPhoneNumber(char* number, int numberSize);
	int GetReceivedSignalStrength();
	bool GetTime(struct tm* tim);

	bool WaitForCSRegistration(long timeout = 120000);
	bool WaitForPSRegistration(long timeout = 120000);
	bool Activate(const char* accessPointName, const char* userName, const char* password, long waitForRegistTimeout = 120000);
	bool Deactivate();

	//bool GetLocation(double* longitude, double* latitude);

	int SocketOpen(const char* host, int port, SocketType type);
	bool SocketSend(int connectId, const byte* data, int dataSize);
	bool SocketSend(int connectId, const char* data);
	int SocketReceive(int connectId, byte* data, int dataSize);
	int SocketReceive(int connectId, char* data, int dataSize);
	int SocketReceive(int connectId, byte* data, int dataSize, long timeout);
	int SocketReceive(int connectId, char* data, int dataSize, long timeout);
	bool SocketClose(int connectId);

	int HttpGet(const char* url, char* data, int dataSize);
	bool HttpPost(const char* url, const char* data, int* responseCode);

	bool SendUSSD(const char* in, char* out, int outSize);

};
