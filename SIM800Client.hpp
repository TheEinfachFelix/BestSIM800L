#pragma once
#include "SIM800.hpp"

class SIM800Client {
public:
    SIM800Client(SIM800& modem);

    SIM800Response testAT(uint32_t timeoutMs = 2000);
    SIM800Response getSignalQuality(uint32_t timeoutMs = 2000);
    SIM800Response getNetworkRegistration(uint32_t timeoutMs = 2000);

    // sendSMS: synchroner Ablauf: CMGF -> CMGS -> (PROMPT) -> send body -> waitForResponse
    SIM800Response sendSMS(const String& number, const String& text, uint32_t timeoutMs = 10000);

private:
    SIM800& _modem;
};
