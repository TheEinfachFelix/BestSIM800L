#pragma once
#include "SIM800.hpp"

constexpr uint32_t DEFAULT_TIMEOUT = 2000;
bool extractAtField(const String& payload, const char* tag, String& out);

struct CSQInfo {
    int rssi;  // 0-31, 99 = unknown
    int ber;   // 0-7, 99 = unknown
};

struct CREGInfo {
    int n;     // mode
    int stat;  // status
};

enum class SIM800PinStatus {
    READY,        // kein PIN nötig
    SIM_PIN,     // PIN nötig
    SIM_PUK,     // PUK nötig
    PH_SIM_PIN,  // Telefon-Sperre (antitheft)
    PH_SIM_PUK,  // Telefon-Sperre (antitheft)
    SIM_PIN2,    // PIN2 nötig
    SIM_PUK2,    // PUK2 nötig
    ERROR,       // Fehler
    UNKNOWN      // unbekannt
};

class SIM800Client {
public:
    SIM800Client(SIM800& modem);

    void enableDebug(Stream& debugOut = Serial);
    void disableDebug();

    SIM800Response testAT(uint32_t timeoutMs = DEFAULT_TIMEOUT);

    SIM800Response setPin(const String& pin, uint32_t timeoutMs = DEFAULT_TIMEOUT, const String& pin2 = "", const String& puk= "", const String& puk2= "");
    SIM800PinStatus getPinStatus(uint32_t timeoutMs = DEFAULT_TIMEOUT);


    SIM800Response getSignalQualityRaw(uint32_t timeoutMs = DEFAULT_TIMEOUT);
    bool getSignalQuality(CSQInfo& out, uint32_t timeoutMs = DEFAULT_TIMEOUT);

    SIM800Response getNetworkRegistrationRaw(uint32_t timeoutMs = DEFAULT_TIMEOUT);
    bool getNetworkRegistration(CREGInfo& out, uint32_t timeoutMs = DEFAULT_TIMEOUT);

    SIM800Response sendSMS(const String& number, const String& text, uint32_t timeoutMs = DEFAULT_TIMEOUT);

private:
    SIM800& _modem;
    bool _debugEnabled;
    Stream* _debugOut;

    void dbgPrint(const String& msg);
    SIM800Response ensureTextMode(uint32_t timeoutMs = DEFAULT_TIMEOUT);
};
