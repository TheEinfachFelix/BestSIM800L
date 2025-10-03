#pragma once
#include "SIM800L.hpp"

class SIM800Client {
public:
    SIM800Client(SIM800& modem);

    void testAT(SIM800::Callback cb);
    void getSignalQuality(SIM800::Callback cb);
    void getNetworkRegistration(SIM800::Callback cb);
    void sendSMS(const String& number, const String& text, SIM800::Callback cb);

private:
    SIM800& _modem;
};