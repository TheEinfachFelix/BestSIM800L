#include "SIM800LClient.hpp"

SIM800Client::SIM800Client(SIM800& modem) : _modem(modem) {}

void SIM800Client::testAT(SIM800::Callback cb) {
    _modem.sendCommand("AT", cb);
}

void SIM800Client::getSignalQuality(SIM800::Callback cb) {
    _modem.sendCommand("AT+CSQ", cb);
}

void SIM800Client::getNetworkRegistration(SIM800::Callback cb) {
    _modem.sendCommand("AT+CREG?", cb);
}

void SIM800Client::sendSMS(const String& number, const String& text, SIM800::Callback cb) {
    _modem.sendCommand("AT+CMGF=1", [=](const String& resp1) {
        if (resp1 == "ERROR" || resp1 == "TIMEOUT") {
            if (cb) cb(resp1);
            return;
        }
        _modem.sendCommand("AT+CMGS=\"" + number + "\"", [=](const String& resp2) {
            if (resp2 == "ERROR" || resp2 == "TIMEOUT") {
                if (cb) cb(resp2);
                return;
            }
            // Text + Ctrl+Z senden
            _modem.sendCommand(text + String((char)26), cb, 5000);
        });
    });
}