#include "SIM800Client.hpp"

SIM800Client::SIM800Client(SIM800& modem) : _modem(modem) {}

SIM800Response SIM800Client::testAT(uint32_t timeoutMs) {
    return _modem.sendCommand("AT", timeoutMs);
}

SIM800Response SIM800Client::getSignalQuality(uint32_t timeoutMs) {
    return _modem.sendCommand("AT+CSQ", timeoutMs);
}

SIM800Response SIM800Client::getNetworkRegistration(uint32_t timeoutMs) {
    return _modem.sendCommand("AT+CREG?", timeoutMs);
}

SIM800Response SIM800Client::sendSMS(const String& number, const String& text, uint32_t timeoutMs) {
    // 1) Set text mode
    _modem.sendCommand("AT+CMGF?", 2000);
    auto r = _modem.sendCommand("AT+CMGF=1", 2000);
    if (r.result != SIM800Result::SUCCESS) return r;

    // 2) Start CMGS -> expect PROMPT '>'
    String cmd = "AT+CMGS=\"" + number + "\"";
    auto r2 = _modem.sendCommand(cmd, 5000);

    if (r2.result == SIM800Result::PROMPT) {
        // send text + Ctrl-Z
        _modem.sendRaw(text + String((char)26));
        // wait for final OK/ERROR
        return _modem.waitForResponse(timeoutMs);
    }

    // sonst: Fehler/Timeout oder unexpected response
    return r2;
}
