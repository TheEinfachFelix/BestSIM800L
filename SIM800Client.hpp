#pragma once
#include "SIM800.hpp"

struct CSQInfo {
    int rssi;  // 0-31, 99 = unknown
    int ber;   // 0-7, 99 = unknown
};

struct CREGInfo {
    int n;     // mode
    int stat;  // status
};

class SIM800Client {
public:
    SIM800Client(SIM800& modem);

    void enableDebug(Stream& debugOut = Serial);
    void disableDebug();

    SIM800Response testAT(uint32_t timeoutMs = 2000);

    SIM800Response getSignalQualityRaw(uint32_t timeoutMs = 2000);
    bool getSignalQuality(CSQInfo& out, uint32_t timeoutMs = 2000);

    SIM800Response getNetworkRegistrationRaw(uint32_t timeoutMs = 2000);
    bool getNetworkRegistration(CREGInfo& out, uint32_t timeoutMs = 2000);

    SIM800Response sendSMS(const String& number, const String& text, uint32_t timeoutMs = 10000);

private:
    SIM800& _modem;
    bool _debugEnabled;
    Stream* _debugOut;

    void dbgPrint(const String& msg);
};
