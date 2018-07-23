#pragma once

#include "SerialAPI.h"
#include "Stopwatch.h"
#include <string>

class Wio3G;

class AtSerial
{
private:
	SerialAPI* _Serial;
	Wio3G* _Wio3G;
	unsigned long _EchoOn;

	bool ReadResponseInternal(const char* pattern, unsigned long timeout, std::string* response, int responseMaxLength);

public:
	AtSerial(SerialAPI* serial, Wio3G* wio3G);

	void SetEcho(bool on);

	bool WaitForAvailable(Stopwatch* sw, unsigned long timeout) const;

	void WriteBinary(const byte* data, int dataSize);
	bool ReadBinary(byte* data, int dataSize, unsigned long timeout);

	void WriteCommand(const char* command);
	bool ReadResponse(const char* pattern, unsigned long timeout, std::string* capture);
	bool WriteCommandAndReadResponse(const char* command, const char* pattern, unsigned long timeout, std::string* capture);

	bool ReadResponseQHTTPREAD(char* data, int dataSize, unsigned long timeout);

};
