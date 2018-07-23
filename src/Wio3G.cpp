#include "Wio3GConfig.h"
#include "Wio3G.h"

#include "Internal/Debug.h"
#include "Internal/StringBuilder.h"
#include "Internal/ArgumentParser.h"
#include "Wio3GHardware.h"
#include <string.h>
#include <limits.h>

#define RET_OK(val)					(ReturnOk(val))
#define RET_ERR(val,err)			(ReturnError(__LINE__, val, err))

#define CONNECT_ID_NUM				(12)
#define POLLING_INTERVAL			(100)

#define HTTP_POST_USER_AGENT		"QUECTEL_MODULE"
#define HTTP_POST_CONTENT_TYPE		"application/json"

#define LINEAR_SCALE(val, inMin, inMax, outMin, outMax)	(((val) - (inMin)) / ((inMax) - (inMin)) * ((outMax) - (outMin)) + (outMin))

////////////////////////////////////////////////////////////////////////////////////////
// Helper functions

static bool SplitUrl(const char* url, const char** host, int* hostLength, const char** uri, int* uriLength)
{
	if (strncmp(url, "http://", 7) == 0) {
		*host = &url[7];
	}
	else if (strncmp(url, "https://", 8) == 0) {
		*host = &url[8];
	}
	else {
		return false;
	}

	const char* ptr;
	for (ptr = *host; *ptr != '\0'; ptr++) {
		if (*ptr == '/') break;
	}
	*hostLength = ptr - *host;
	*uri = ptr;
	*uriLength = strlen(ptr);

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////
// Wio3G

bool Wio3G::ReturnError(int lineNumber, bool value, Wio3G::ErrorCodeType errorCode)
{
	_LastErrorCode = errorCode;

	char str[100];
	sprintf(str, "%d", lineNumber);
	DEBUG_PRINT("ERROR! ");
	DEBUG_PRINTLN(str);

	return value;
}

int Wio3G::ReturnError(int lineNumber, int value, Wio3G::ErrorCodeType errorCode)
{
	_LastErrorCode = errorCode;

	char str[100];
	sprintf(str, "%d", lineNumber);
	DEBUG_PRINT("ERROR! ");
	DEBUG_PRINTLN(str);

	return value;
}

bool Wio3G::IsBusy() const
{
	return digitalRead(MODULE_STATUS_PIN) ? false : true;
}

bool Wio3G::IsRespond()
{
	Stopwatch sw;
	sw.Restart();
	while (!_AtSerial.WriteCommandAndReadResponse("AT", "^OK$", 500, NULL)) {
		if (sw.ElapsedMilliseconds() >= 2000) return false;
	}

	return true;
}

bool Wio3G::Reset()
{
	digitalWrite(MODULE_RESET_PIN, HIGH);
	delay(200);
	digitalWrite(MODULE_RESET_PIN, LOW);
	delay(300);

	return true;
}

bool Wio3G::TurnOn()
{
	delay(100);
	digitalWrite(MODULE_PWRKEY_PIN, HIGH);
	delay(200);
	digitalWrite(MODULE_PWRKEY_PIN, LOW);

	return true;
}

bool Wio3G::HttpSetUrl(const char* url)
{
	StringBuilder str;
	if (!str.WriteFormat("AT+QHTTPURL=%d", strlen(url))) return false;
	_AtSerial.WriteCommand(str.GetString());
	if (!_AtSerial.ReadResponse("^CONNECT$", 500, NULL)) return false;

	_AtSerial.WriteBinary((const byte*)url, strlen(url));
	if (!_AtSerial.ReadResponse("^OK$", 500, NULL)) return false;

	return true;
}

bool Wio3G::ReadResponseCallback(const char* response)
{
	return false;
}

Wio3G::Wio3G() : _SerialAPI(&SerialModule), _AtSerial(&_SerialAPI, this), _Led()
{
}

Wio3G::ErrorCodeType Wio3G::GetLastError() const
{
	return _LastErrorCode;
}

void Wio3G::Init()
{
	////////////////////
	// Module

	// Power Supply
	pinMode(MODULE_PWR_PIN, OUTPUT); digitalWrite(MODULE_PWR_PIN, LOW);
	// Turn On/Off
	pinMode(MODULE_PWRKEY_PIN, OUTPUT); digitalWrite(MODULE_PWRKEY_PIN, LOW);
	pinMode(MODULE_RESET_PIN, OUTPUT); digitalWrite(MODULE_RESET_PIN, LOW);
	// Status Indication
	pinMode(MODULE_STATUS_PIN, INPUT_PULLUP);
	// Main UART Interface
	pinMode(MODULE_DTR_PIN, OUTPUT); digitalWrite(MODULE_DTR_PIN, LOW);

	SerialModule.begin(115200);

	////////////////////
	// Led

	pinMode(LED_VDD_PIN, OUTPUT); digitalWrite(LED_VDD_PIN, LOW);
	pinMode(LED_PIN, OUTPUT); digitalWrite(LED_PIN, HIGH);

	////////////////////
	// Grove

	pinMode(GROVE_VCCB_PIN, OUTPUT); digitalWrite(GROVE_VCCB_PIN, LOW);
}

void Wio3G::PowerSupplyCellular(bool on)
{
	digitalWrite(MODULE_PWR_PIN, on ? HIGH : LOW);
}

void Wio3G::PowerSupplyLed(bool on)
{
	digitalWrite(LED_VDD_PIN, on ? HIGH : LOW);
}

void Wio3G::PowerSupplyGrove(bool on)
{
	digitalWrite(GROVE_VCCB_PIN, on ? HIGH : LOW);
}

void Wio3G::LedSetRGB(uint8_t red, uint8_t green, uint8_t blue)
{
	_Led.Reset();
	_Led.SetSingleLED(red, green, blue);
}

bool Wio3G::TurnOnOrReset()
{
	std::string response;

	if (IsRespond()) {
		DEBUG_PRINTLN("Reset()");
		if (!Reset()) return RET_ERR(false, E_UNKNOWN);
	}
	else {
		DEBUG_PRINTLN("TurnOn()");
		if (!TurnOn()) return RET_ERR(false, E_UNKNOWN);
	}

	Stopwatch sw;
	sw.Restart();
	while (!_AtSerial.WriteCommandAndReadResponse("AT", "^OK$", 500, NULL)) {
		DEBUG_PRINT(".");
		if (sw.ElapsedMilliseconds() >= 10000) return RET_ERR(false, E_UNKNOWN);
	}
	DEBUG_PRINTLN("");

	if (!_AtSerial.WriteCommandAndReadResponse("ATE0", "^OK$", 500, NULL)) return RET_ERR(false, E_UNKNOWN);
	_AtSerial.SetEcho(false);

	sw.Restart();
	while (true) {
		if (!_AtSerial.WriteCommandAndReadResponse("AT+CPIN?", "^(OK|\\+CME ERROR: .*)$", 5000, &response)) return RET_ERR(false, E_UNKNOWN);
		if (response == "OK") break;
		if (sw.ElapsedMilliseconds() >= 10000) return RET_ERR(false, E_UNKNOWN);
		delay(POLLING_INTERVAL);
	}

	return true;
}

bool Wio3G::TurnOff()
{
	if (!_AtSerial.WriteCommandAndReadResponse("AT+QPOWD", "^OK$", 500, NULL)) return RET_ERR(false, E_UNKNOWN);
	if (!_AtSerial.ReadResponse("^POWERED DOWN$", 60000, NULL)) return RET_ERR(false, E_UNKNOWN);

	return RET_OK(true);
}

int Wio3G::GetIMEI(char* imei, int imeiSize)
{
	std::string response;
	std::string imeiStr;

	_AtSerial.WriteCommand("AT+GSN");
	while (true) {
		if (!_AtSerial.ReadResponse("^(OK|[0-9]+)$", 500, &response)) return RET_ERR(-1, E_UNKNOWN);
		if (response == "OK") break;
		imeiStr = response;
	}

	if ((int)imeiStr.size() + 1 > imeiSize) return RET_ERR(-1, E_UNKNOWN);
	strcpy(imei, imeiStr.c_str());

	return RET_OK((int)strlen(imei));
}

int Wio3G::GetIMSI(char* imsi, int imsiSize)
{
	std::string response;
	std::string imsiStr;

	_AtSerial.WriteCommand("AT+CIMI");
	while (true) {
		if (!_AtSerial.ReadResponse("^(OK|[0-9]+)$", 500, &response)) return RET_ERR(-1, E_UNKNOWN);
		if (response == "OK") break;
		imsiStr = response;
	}

	if ((int)imsiStr.size() + 1 > imsiSize) return RET_ERR(-1, E_UNKNOWN);
	strcpy(imsi, imsiStr.c_str());

	return RET_OK((int)strlen(imsi));
}

int Wio3G::GetPhoneNumber(char* number, int numberSize)
{
	std::string response;
	ArgumentParser parser;
	std::string numberStr;

	_AtSerial.WriteCommand("AT+CNUM");
	while (true) {
		if (!_AtSerial.ReadResponse("^(OK|\\+CNUM: .*)$", 500, &response)) return RET_ERR(-1, E_UNKNOWN);
		if (response == "OK") break;

		if (numberStr.size() >= 1) continue;

		parser.Parse(response.c_str());
		if (parser.Size() < 2) return RET_ERR(-1, E_UNKNOWN);
		numberStr = parser[1];
	}

	if ((int)numberStr.size() + 1 > numberSize) return RET_ERR(-1, E_UNKNOWN);
	strcpy(number, numberStr.c_str());

	return RET_OK((int)strlen(number));
}

int Wio3G::GetReceivedSignalStrength()
{
	std::string response;
	ArgumentParser parser;

	_AtSerial.WriteCommand("AT+CSQ");
	if (!_AtSerial.ReadResponse("^\\+CSQ: (.*)$", 500, &response)) return RET_ERR(INT_MIN, E_UNKNOWN);

	parser.Parse(response.c_str());
	if (parser.Size() != 2) return RET_ERR(INT_MIN, E_UNKNOWN);
	int rssi = atoi(parser[0]);

	if (!_AtSerial.ReadResponse("^OK$", 500, NULL)) return RET_ERR(INT_MIN, E_UNKNOWN);

	if (rssi == 0) return RET_OK(-113);
	else if (rssi == 1) return RET_OK(-111);
	else if (2 <= rssi && rssi <= 30) return RET_OK((int)LINEAR_SCALE((double)rssi, 2, 30, -109, -53));
	else if (rssi == 31) return RET_OK(-51);
	else if (rssi == 99) return RET_OK(-999);

	return RET_OK(-999);
}

bool Wio3G::GetTime(struct tm* tim)
{
	std::string response;

	_AtSerial.WriteCommand("AT+QLTS=1");
	if (!_AtSerial.ReadResponse("^\\+QLTS: (.*)$", 500, &response)) return RET_ERR(false, E_UNKNOWN);
	if (!_AtSerial.ReadResponse("^OK$", 500, NULL)) return RET_ERR(false, E_UNKNOWN);

	if (strlen(response.c_str()) != 24) return RET_ERR(false, E_UNKNOWN);
	const char* parameter = response.c_str();

	if (parameter[0] != '"') return RET_ERR(false, E_UNKNOWN);
	if (parameter[3] != '/') return RET_ERR(false, E_UNKNOWN);
	if (parameter[6] != '/') return RET_ERR(false, E_UNKNOWN);
	if (parameter[9] != ',') return RET_ERR(false, E_UNKNOWN);
	if (parameter[12] != ':') return RET_ERR(false, E_UNKNOWN);
	if (parameter[15] != ':') return RET_ERR(false, E_UNKNOWN);
	if (parameter[21] != ',') return RET_ERR(false, E_UNKNOWN);
	if (parameter[23] != '"') return RET_ERR(false, E_UNKNOWN);

	int yearOffset = atoi(&parameter[1]);
	tim->tm_year = (yearOffset >= 80 ? 1900 : 2000) + yearOffset - 1900;
	tim->tm_mon = atoi(&parameter[4]) - 1;
	tim->tm_mday = atoi(&parameter[7]);
	tim->tm_hour = atoi(&parameter[10]);
	tim->tm_min = atoi(&parameter[13]);
	tim->tm_sec = atoi(&parameter[16]);
	tim->tm_wday = 0;
	tim->tm_yday = 0;
	tim->tm_isdst = -1;

	return RET_OK(true);
}

bool Wio3G::WaitForCSRegistration(long timeout)
{
	std::string response;
	ArgumentParser parser;

	Stopwatch sw;
	sw.Restart();
	while (true) {
		int status;

		_AtSerial.WriteCommand("AT+CREG?");
		if (!_AtSerial.ReadResponse("^\\+CREG: (.*)$", 500, &response)) return RET_ERR(false, E_UNKNOWN);
		parser.Parse(response.c_str());
		if (parser.Size() < 2) return RET_ERR(false, E_UNKNOWN);
		//resultCode = atoi(parser[0]);
		status = atoi(parser[1]);
		if (!_AtSerial.ReadResponse("^OK$", 500, NULL)) return RET_ERR(false, E_UNKNOWN);
		if (status == 0) return RET_ERR(false, E_UNKNOWN);
		if (status == 1 || status == 5) break;

		if (sw.ElapsedMilliseconds() >= (unsigned long)timeout) return RET_ERR(false, E_UNKNOWN);
	}

	// for debug.
#ifdef WIO_DEBUG
	char str[100];
	sprintf(str, "Elapsed time is %lu[msec.].", sw.ElapsedMilliseconds());
	DEBUG_PRINTLN(str);
#endif // WIO_DEBUG

	return RET_OK(true);
}

bool Wio3G::WaitForPSRegistration(long timeout)
{
	std::string response;
	ArgumentParser parser;

	Stopwatch sw;
	sw.Restart();
	while (true) {
		int status;

		_AtSerial.WriteCommand("AT+CGREG?");
		if (!_AtSerial.ReadResponse("^\\+CGREG: (.*)$", 500, &response)) return RET_ERR(false, E_UNKNOWN);
		parser.Parse(response.c_str());
		if (parser.Size() < 2) return RET_ERR(false, E_UNKNOWN);
		//resultCode = atoi(parser[0]);
		status = atoi(parser[1]);
		if (!_AtSerial.ReadResponse("^OK$", 500, NULL)) return RET_ERR(false, E_UNKNOWN);
		if (status == 0) return RET_ERR(false, E_UNKNOWN);
		if (status == 1 || status == 5) break;

		if (sw.ElapsedMilliseconds() >= (unsigned long)timeout) return RET_ERR(false, E_UNKNOWN);
	}

	// for debug.
#ifdef WIO_DEBUG
	char str[100];
	sprintf(str, "Elapsed time is %lu[msec.].", sw.ElapsedMilliseconds());
	DEBUG_PRINTLN(str);
#endif // WIO_DEBUG

	return RET_OK(true);
}

bool Wio3G::Activate(const char* accessPointName, const char* userName, const char* password, long waitForRegistTimeout)
{
	std::string response;
	ArgumentParser parser;

	if (!WaitForPSRegistration(waitForRegistTimeout)) return RET_ERR(false, E_UNKNOWN);

	// for debug.
#ifdef WIO_DEBUG
	_AtSerial.WriteCommandAndReadResponse("AT+CREG?", "^OK$", 500, NULL);
	_AtSerial.WriteCommandAndReadResponse("AT+CGREG?", "^OK$", 500, NULL);
#endif // WIO_DEBUG

	StringBuilder str;
	if (!str.WriteFormat("AT+QICSGP=1,1,\"%s\",\"%s\",\"%s\",1", accessPointName, userName, password)) return RET_ERR(false, E_UNKNOWN);
	if (!_AtSerial.WriteCommandAndReadResponse(str.GetString(), "^OK$", 500, NULL)) return RET_ERR(false, E_UNKNOWN);

	Stopwatch sw;
	sw.Restart();
	while (true) {
		_AtSerial.WriteCommand("AT+QIACT=1");
		if (!_AtSerial.ReadResponse("^(OK|ERROR)$", 150000, &response)) return RET_ERR(false, E_UNKNOWN);
		if (response == "OK") break;
		if (!_AtSerial.WriteCommandAndReadResponse("AT+QIGETERROR", "^OK$", 500, NULL)) return RET_ERR(false, E_UNKNOWN);
		if (sw.ElapsedMilliseconds() >= 150000) return RET_ERR(false, E_UNKNOWN);
		delay(POLLING_INTERVAL);
	}

	// for debug.
#ifdef WIO_DEBUG
	if (!_AtSerial.WriteCommandAndReadResponse("AT+QIACT?", "^OK$", 150000, NULL)) return RET_ERR(false, E_UNKNOWN);
#endif // WIO_DEBUG

	return RET_OK(true);
}

bool Wio3G::Deactivate()
{
	if (!_AtSerial.WriteCommandAndReadResponse("AT+QIDEACT=1", "^OK$", 40000, NULL)) return RET_ERR(false, E_UNKNOWN);

	return RET_OK(true);
}

int Wio3G::SocketOpen(const char* host, int port, SocketType type)
{
	std::string response;
	ArgumentParser parser;

	if (host == NULL || host[0] == '\0') return RET_ERR(-1, E_UNKNOWN);
	if (port < 0 || 65535 < port) return RET_ERR(-1, E_UNKNOWN);

	const char* typeStr;
	switch (type) {
	case SOCKET_TCP:
		typeStr = "TCP";
		break;
	case SOCKET_UDP:
		typeStr = "UDP";
		break;
	default:
		return RET_ERR(-1, E_UNKNOWN);
	}

	bool connectIdUsed[CONNECT_ID_NUM];
	for (int i = 0; i < CONNECT_ID_NUM; i++) connectIdUsed[i] = false;

	_AtSerial.WriteCommand("AT+QISTATE?");
	do {
		if (!_AtSerial.ReadResponse("^(OK|\\+QISTATE: .*)$", 10000, &response)) return RET_ERR(-1, E_UNKNOWN);
		if (strncmp(response.c_str(), "+QISTATE: ", 10) == 0) {
			parser.Parse(&response.c_str()[10]);
			if (parser.Size() >= 1) {
				int connectId = atoi(parser[0]);
				if (connectId < 0 || CONNECT_ID_NUM <= connectId) return RET_ERR(-1, E_UNKNOWN);
				connectIdUsed[connectId] = true;
			}
		}
	} while (response != "OK");

	int connectId;
	for (connectId = 0; connectId < CONNECT_ID_NUM; connectId++) {
		if (!connectIdUsed[connectId]) break;
	}
	if (connectId >= CONNECT_ID_NUM) return RET_ERR(-1, E_UNKNOWN);

	StringBuilder str;
	if (!str.WriteFormat("AT+QIOPEN=1,%d,\"%s\",\"%s\",%d", connectId, typeStr, host, port)) return RET_ERR(-1, E_UNKNOWN);
	if (!_AtSerial.WriteCommandAndReadResponse(str.GetString(), "^OK$", 150000, NULL)) return RET_ERR(-1, E_UNKNOWN);
	str.Clear();
	if (!str.WriteFormat("^\\+QIOPEN: %d,0$", connectId)) return RET_ERR(-1, E_UNKNOWN);
	if (!_AtSerial.ReadResponse(str.GetString(), 150000, NULL)) return RET_ERR(-1, E_UNKNOWN);

	return RET_OK(connectId);
}

bool Wio3G::SocketSend(int connectId, const byte* data, int dataSize)
{
	if (connectId >= CONNECT_ID_NUM) return RET_ERR(false, E_UNKNOWN);
	if (dataSize > 1460) return RET_ERR(false, E_UNKNOWN);

	StringBuilder str;
	if (!str.WriteFormat("AT+QISEND=%d,%d", connectId, dataSize)) return RET_ERR(false, E_UNKNOWN);
	_AtSerial.WriteCommand(str.GetString());
	if (!_AtSerial.ReadResponse("^>", 500, NULL)) return RET_ERR(false, E_UNKNOWN);
	_AtSerial.WriteBinary(data, dataSize);
	if (!_AtSerial.ReadResponse("^SEND OK$", 5000, NULL)) return RET_ERR(false, E_UNKNOWN);

	return RET_OK(true);
}

bool Wio3G::SocketSend(int connectId, const char* data)
{
	return SocketSend(connectId, (const byte*)data, strlen(data));
}

int Wio3G::SocketReceive(int connectId, byte* data, int dataSize)
{
	std::string response;

	if (connectId >= CONNECT_ID_NUM) return RET_ERR(-1, E_UNKNOWN);

	StringBuilder str;
	if (!str.WriteFormat("AT+QIRD=%d", connectId)) return RET_ERR(-1, E_UNKNOWN);
	_AtSerial.WriteCommand(str.GetString());
	if (!_AtSerial.ReadResponse("^\\+QIRD: (.*)$", 500, &response)) return RET_ERR(-1, E_UNKNOWN);
	int dataLength = atoi(response.c_str());
	if (dataLength >= 1) {
		if (dataLength > dataSize) return RET_ERR(-1, E_UNKNOWN);
		if (!_AtSerial.ReadBinary(data, dataLength, 500)) return RET_ERR(-1, E_UNKNOWN);
	}
	if (!_AtSerial.ReadResponse("^OK$", 500, NULL)) return RET_ERR(-1, E_UNKNOWN);

	return RET_OK(dataLength);
}

int Wio3G::SocketReceive(int connectId, char* data, int dataSize)
{
	int dataLength = SocketReceive(connectId, (byte*)data, dataSize - 1);
	if (dataLength >= 0) data[dataLength] = '\0';

	return dataLength;
}

int Wio3G::SocketReceive(int connectId, byte* data, int dataSize, long timeout)
{
	Stopwatch sw;
	sw.Restart();
	int dataLength;
	while ((dataLength = SocketReceive(connectId, data, dataSize)) == 0) {
		if (sw.ElapsedMilliseconds() >= (unsigned long)timeout) return 0;
		delay(POLLING_INTERVAL);
	}
	return dataLength;
}

int Wio3G::SocketReceive(int connectId, char* data, int dataSize, long timeout)
{
	Stopwatch sw;
	sw.Restart();
	int dataLength;
	while ((dataLength = SocketReceive(connectId, data, dataSize)) == 0) {
		if (sw.ElapsedMilliseconds() >= (unsigned long)timeout) return 0;
		delay(POLLING_INTERVAL);
	}
	return dataLength;
}

bool Wio3G::SocketClose(int connectId)
{
	if (connectId >= CONNECT_ID_NUM) return RET_ERR(false, E_UNKNOWN);

	StringBuilder str;
	if (!str.WriteFormat("AT+QICLOSE=%d", connectId)) return RET_ERR(false, E_UNKNOWN);
	if (!_AtSerial.WriteCommandAndReadResponse(str.GetString(), "^OK$", 10000, NULL)) return RET_ERR(false, E_UNKNOWN);

	return RET_OK(true);
}

int Wio3G::HttpGet(const char* url, char* data, int dataSize)
{
	std::string response;
	ArgumentParser parser;

	if (strncmp(url, "https:", 6) == 0) {
		if (!_AtSerial.WriteCommandAndReadResponse("AT+QHTTPCFG=\"sslctxid\",1", "^OK$", 500, NULL)) return RET_ERR(-1, E_UNKNOWN);
		if (!_AtSerial.WriteCommandAndReadResponse("AT+QSSLCFG=\"sslversion\",1,4", "^OK$", 500, NULL)) return RET_ERR(-1, E_UNKNOWN);
		if (!_AtSerial.WriteCommandAndReadResponse("AT+QSSLCFG=\"ciphersuite\",1,\"0XFFFF\"", "^OK$", 500, NULL)) return RET_ERR(-1, E_UNKNOWN);
		if (!_AtSerial.WriteCommandAndReadResponse("AT+QSSLCFG=\"seclevel\",1,0", "^OK$", 500, NULL)) return RET_ERR(-1, E_UNKNOWN);
	}

	if (!_AtSerial.WriteCommandAndReadResponse("AT+QHTTPCFG=\"requestheader\",0", "^OK$", 500, NULL)) return RET_ERR(-1, E_UNKNOWN);

	if (!HttpSetUrl(url)) return RET_ERR(-1, E_UNKNOWN);

	if (!_AtSerial.WriteCommandAndReadResponse("AT+QHTTPGET", "^OK$", 500, NULL)) return RET_ERR(-1, E_UNKNOWN);
	if (!_AtSerial.ReadResponse("^\\+QHTTPGET: (.*)$", 60000, &response)) return RET_ERR(-1, E_UNKNOWN);

	parser.Parse(response.c_str());
	if (parser.Size() < 1) return RET_ERR(-1, E_UNKNOWN);
	if (strcmp(parser[0], "0") != 0) return RET_ERR(-1, E_UNKNOWN);
	int contentLength = parser.Size() >= 3 ? atoi(parser[2]) : -1;

	_AtSerial.WriteCommand("AT+QHTTPREAD");
	if (!_AtSerial.ReadResponse("^CONNECT$", 1000, NULL)) return RET_ERR(-1, E_UNKNOWN);
	if (contentLength >= 0) {
		if (contentLength + 1 > dataSize) return RET_ERR(-1, E_UNKNOWN);
		if (!_AtSerial.ReadBinary((byte*)data, contentLength, 60000)) return RET_ERR(-1, E_UNKNOWN);
		data[contentLength] = '\0';

		if (!_AtSerial.ReadResponse("^OK$", 1000, NULL)) return RET_ERR(-1, E_UNKNOWN);
	}
	else {
		if (!_AtSerial.ReadResponseQHTTPREAD(data, dataSize, 60000)) return RET_ERR(-1, E_UNKNOWN);
		contentLength = strlen(data);
	}
	if (!_AtSerial.ReadResponse("^\\+QHTTPREAD: 0$", 1000, NULL)) return RET_ERR(-1, E_UNKNOWN);

	return RET_OK(contentLength);
}

bool Wio3G::HttpPost(const char* url, const char* data, int* responseCode)
{
	std::string response;
	ArgumentParser parser;

	if (strncmp(url, "https:", 6) == 0) {
		if (!_AtSerial.WriteCommandAndReadResponse("AT+QHTTPCFG=\"sslctxid\",1", "^OK$", 500, NULL)) return RET_ERR(false, E_UNKNOWN);
		if (!_AtSerial.WriteCommandAndReadResponse("AT+QSSLCFG=\"sslversion\",1,4", "^OK$", 500, NULL)) return RET_ERR(false, E_UNKNOWN);
		if (!_AtSerial.WriteCommandAndReadResponse("AT+QSSLCFG=\"ciphersuite\",1,\"0XFFFF\"", "^OK$", 500, NULL)) return RET_ERR(false, E_UNKNOWN);
		if (!_AtSerial.WriteCommandAndReadResponse("AT+QSSLCFG=\"seclevel\",1,0", "^OK$", 500, NULL)) return RET_ERR(false, E_UNKNOWN);
	}

	if (!_AtSerial.WriteCommandAndReadResponse("AT+QHTTPCFG=\"requestheader\",1", "^OK$", 500, NULL)) return RET_ERR(false, E_UNKNOWN);

	if (!HttpSetUrl(url)) return RET_ERR(false, E_UNKNOWN);

	const char* host;
	int hostLength;
	const char* uri;
	int uriLength;
	if (!SplitUrl(url, &host, &hostLength, &uri, &uriLength)) return RET_ERR(false, E_UNKNOWN);


	StringBuilder header;
	header.Write("POST ");
	if (uriLength <= 0) {
		header.Write("/");
	}
	else {
		header.Write(uri, uriLength);
	}
	header.Write(" HTTP/1.1\r\n");
	header.Write("Host: ");
	header.Write(host, hostLength);
	header.Write("\r\n");
	header.Write("Accept: */*\r\n");
	header.Write("User-Agent: " HTTP_POST_USER_AGENT "\r\n");
	header.Write("Connection: Keep-Alive\r\n");
	header.Write("Content-Type: " HTTP_POST_CONTENT_TYPE "\r\n");
	if (!header.WriteFormat("Content-Length: %d\r\n", strlen(data))) return RET_ERR(false, E_UNKNOWN);
	header.Write("\r\n");

	StringBuilder str;
	if (!str.WriteFormat("AT+QHTTPPOST=%d", header.Length() + strlen(data))) return RET_ERR(false, E_UNKNOWN);
	_AtSerial.WriteCommand(str.GetString());
	if (!_AtSerial.ReadResponse("^CONNECT$", 60000, NULL)) return RET_ERR(false, E_UNKNOWN);
	const char* headerStr = header.GetString();
	_AtSerial.WriteBinary((const byte*)headerStr, strlen(headerStr));
	_AtSerial.WriteBinary((const byte*)data, strlen(data));
	if (!_AtSerial.ReadResponse("^OK$", 1000, NULL)) return RET_ERR(false, E_UNKNOWN);
	if (!_AtSerial.ReadResponse("^\\+QHTTPPOST: (.*)$", 60000, &response)) return RET_ERR(false, E_UNKNOWN);
	parser.Parse(response.c_str());
	if (parser.Size() < 1) return RET_ERR(false, E_UNKNOWN);
	if (strcmp(parser[0], "0") != 0) return RET_ERR(false, E_UNKNOWN);
	if (parser.Size() < 2) {
		*responseCode = -1;
	}
	else {
		*responseCode = atoi(parser[1]);
	}

	return RET_OK(true);
}

//! Send a USSD message.
/*!
  \param in    a pointer to an input string (ASCII characters) which will be sent to SORACOM Beam/Funnel/Harvest
	             after converted to GSM default 7 bit alphabets. allowed up to 182 characters.
  \param out   a pointer to an output buffer to receive response message.
  \param outSize specify allocated size of `out` in bytes.
*/
bool Wio3G::SendUSSD(const char* in, char* out, int outSize)
{
	if (in == NULL || out == NULL) {
		return RET_ERR(false, E_UNKNOWN);
	}
	if (strlen(in) > 182) {
		DEBUG_PRINTLN("the maximum size of a USSD message is 182 characters.");
		return RET_ERR(false, E_UNKNOWN);
	}

	StringBuilder str;
	if (!str.WriteFormat("AT+CUSD=1,\"%s\"", in)) {
		DEBUG_PRINTLN("error while sending 'AT+CUSD'");
		return RET_ERR(false, E_UNKNOWN);
	}
	_AtSerial.WriteCommand(str.GetString());

	std::string response;
	if (!_AtSerial.ReadResponse("^\\+CUSD: [0-9],\"(.*)\",[0-9]+$", 120000, &response)) {
		DEBUG_PRINTLN("error while reading response of 'AT+CUSD'");
		return RET_ERR(false, E_UNKNOWN);
	}

	if ((int)response.size() + 1 > outSize) return RET_ERR(false, E_UNKNOWN);
	strcpy(out, response.c_str());

	return RET_OK(true);
}
