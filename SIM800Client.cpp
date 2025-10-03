#include "SIM800Client.hpp"

SIM800Client::SIM800Client(SIM800& modem) 
    : _modem(modem), _debugEnabled(false), _debugOut(nullptr) {}

void SIM800Client::enableDebug(Stream& debugOut) {
    _debugEnabled = true;
    _debugOut = &debugOut;
}
void SIM800Client::disableDebug() { _debugEnabled = false; _debugOut = nullptr; }

void SIM800Client::dbgPrint(const String& msg) {
    if (_debugEnabled && _debugOut) _debugOut->println("[CLIENT] " + msg);
}

SIM800Response SIM800Client::testAT(uint32_t timeoutMs) {
    dbgPrint("AT");
    return _modem.sendCommand("AT", timeoutMs);
}

SIM800Response SIM800Client::getSignalQualityRaw(uint32_t timeoutMs) {
    dbgPrint("AT+CSQ");
    return _modem.sendCommand("AT+CSQ", timeoutMs);
}

bool SIM800Client::getSignalQuality(CSQInfo& out, uint32_t timeoutMs) {
    auto r = getSignalQualityRaw(timeoutMs);
    if (r.result != SIM800Result::SUCCESS) return false;

    // payload z.B. "+CSQ: 17,99"
    int idx = r.payload.indexOf("+CSQ:");
    if (idx >= 0) {
        int comma = r.payload.indexOf(',', idx);
        if (comma > 0) {
            out.rssi = r.payload.substring(idx+5, comma).toInt();
            out.ber  = r.payload.substring(comma+1).toInt();
            dbgPrint("CSQ parsed: rssi=" + String(out.rssi) + " ber=" + String(out.ber));
            return true;
        }
    }
    return false;
}

SIM800Response SIM800Client::getNetworkRegistrationRaw(uint32_t timeoutMs) {
    dbgPrint("AT+CREG?");
    return _modem.sendCommand("AT+CREG?", timeoutMs);
}

bool SIM800Client::getNetworkRegistration(CREGInfo& out, uint32_t timeoutMs) {
    auto r = getNetworkRegistrationRaw(timeoutMs);
    if (r.result != SIM800Result::SUCCESS) return false;

    // payload z.B. "+CREG: 0,1"
    int idx = r.payload.indexOf("+CREG:");
    if (idx >= 0) {
        int comma = r.payload.indexOf(',', idx);
        if (comma > 0) {
            out.n    = r.payload.substring(idx+6, comma).toInt();
            out.stat = r.payload.substring(comma+1).toInt();
            dbgPrint("CREG parsed: n=" + String(out.n) + " stat=" + String(out.stat));
            return true;
        }
    }
    return false;
}

SIM800Response SIM800Client::sendSMS(const String& number, const String& text, uint32_t timeoutMs) {
    dbgPrint("sendSMS to " + number);

    auto r = _modem.sendCommand("AT+CMGF=1", 2000);
    if (r.result != SIM800Result::SUCCESS) return r;

    auto r2 = _modem.sendCommand("AT+CMGS=\"" + number + "\"", 5000);
    if (r2.result == SIM800Result::PROMPT) {
        _modem.sendRaw(text + String((char)26));
        return _modem.waitForResponse(timeoutMs);
    }
    return r2;
}
