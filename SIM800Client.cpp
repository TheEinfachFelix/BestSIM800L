#include "SIM800Client.hpp"

static const std::map<String, SIM800PinStatus> pinStatusMap = {
    {"READY", SIM800PinStatus::READY},
    {"SIM PIN", SIM800PinStatus::SIM_PIN},
    {"SIM PUK", SIM800PinStatus::SIM_PUK},
    {"PH_SIM PIN", SIM800PinStatus::PH_SIM_PIN},
    {"PH_SIM PUK", SIM800PinStatus::PH_SIM_PUK},
    {"SIM PIN2", SIM800PinStatus::SIM_PIN2},
};

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

SIM800Response SIM800Client::setPin(const String& pin, uint32_t timeoutMs, const String& pin2, const String& puk, const String& puk2) {
    auto status = getPinStatus(timeoutMs);
    if (status == SIM800PinStatus::READY) {
        dbgPrint("SIM already unlocked");
        return SIM800Response{SIM800Result::SUCCESS, "SIM already unlocked"};
    }
    if (status == SIM800PinStatus::SIM_PIN) {
        dbgPrint("Set PIN");
        return _modem.sendCommand("AT+CPIN=\"" + pin + "\"", timeoutMs);
    }
    if (status == SIM800PinStatus::SIM_PUK) {
        if (puk.length() == 0 || pin.length() == 0) {
            dbgPrint("PUK required but not provided");
            return SIM800Response{SIM800Result::ERROR, "PUK required but not provided"};
        }
        dbgPrint("Set PUK and new PIN");
        return _modem.sendCommand("AT+CPIN=\"" + puk + "\",\"" + pin + "\"", timeoutMs);
    }
    if (status == SIM800PinStatus::SIM_PIN2) {
        if (pin2.length() == 0) {
            dbgPrint("PIN2 required but not provided");
            return SIM800Response{SIM800Result::ERROR, "PIN2 required but not provided"};
        }
        dbgPrint("Set PIN2");
        return _modem.sendCommand("AT+CPIN=\"" + pin2 + "\"", timeoutMs);
    }
    if (status == SIM800PinStatus::SIM_PUK2) {
        if (puk2.length() == 0 || pin.length() == 0) {
            dbgPrint("PUK2 required but not provided");
            return SIM800Response{SIM800Result::ERROR, "PUK2 required but not provided"};
        }
        dbgPrint("Set PUK2 and new PIN2");
        return _modem.sendCommand("AT+CPIN=\"" + puk2 + "\",\"" + pin + "\"", timeoutMs);
    }
    dbgPrint("Error getting PIN status");
    return SIM800Response{SIM800Result::ERROR, "Error getting PIN status"};
}

SIM800PinStatus SIM800Client::getPinStatus(uint32_t timeoutMs) {
    auto r = _modem.sendCommand("AT+CPIN?", timeoutMs);
    if (r.result == SIM800Result::SUCCESS) {

        String field;
        if (!extractAtField(r.payload, "+CPIN", field)) {
            dbgPrint("Failed to parse CPIN response");
            return SIM800PinStatus::UNKNOWN;
        }

        field.trim();
        dbgPrint("PIN status: " + field);

        auto it = pinStatusMap.find(field);
        if (it != pinStatusMap.end()) return it->second;
        
    } else if (r.result == SIM800Result::ERROR )
    {
        dbgPrint("Error getting PIN status");
        dbgPrint("Error code: " + r.payload);
        return SIM800PinStatus::ERROR;
    }
    
    dbgPrint("Failed to get PIN status");
    return SIM800PinStatus::UNKNOWN;
}

SIM800Response SIM800Client::getSignalQualityRaw(uint32_t timeoutMs) {
    dbgPrint("AT+CSQ");
    return _modem.sendCommand("AT+CSQ", timeoutMs);
}

bool SIM800Client::getSignalQuality(CSQInfo& out, uint32_t timeoutMs) {
    auto r = getSignalQualityRaw(timeoutMs);
    if (r.result != SIM800Result::SUCCESS) return false;

    String field;
    if (!extractAtField(r.payload, "+CSQ", field)) return false;

    int comma = field.indexOf(',');
    if (comma < 0) return false;

    out.rssi = field.substring(0, comma).toInt();
    out.ber  = field.substring(comma + 1).toInt();
    
    dbgPrint("CSQ parsed: rssi=" + String(out.rssi) + " ber=" + String(out.ber));
    return true;
}

SIM800Response SIM800Client::getNetworkRegistrationRaw(uint32_t timeoutMs) {
    dbgPrint("AT+CREG?");
    return _modem.sendCommand("AT+CREG?", timeoutMs);
}

bool SIM800Client::getNetworkRegistration(CREGInfo& out, uint32_t timeoutMs) {
    auto r = getNetworkRegistrationRaw(timeoutMs);
    if (r.result != SIM800Result::SUCCESS) return false;

    String field;
    if (!extractAtField(r.payload, "+CREG", field)) return false;

    int comma = field.indexOf(',');
    if (comma < 0) return false;

    out.n    = field.substring(0, comma).toInt();
    out.stat = field.substring(comma + 1).toInt();

    dbgPrint("CREG parsed: n=" + String(out.n) + " stat=" + String(out.stat));
    return true;
}

SIM800Response SIM800Client::ensureTextMode(uint32_t timeoutMs) {
    auto r = _modem.sendCommand("AT+CMGF?", timeoutMs);
    if (r.result != SIM800Result::SUCCESS) return r;
    if (r.payload.indexOf("+CMGF: 1") < 0)
        dbgPrint("Set text mode");
        return _modem.sendCommand("AT+CMGF=1", timeoutMs);
    return r;
}

SIM800Response SIM800Client::sendSMS(const String& number, const String& text, uint32_t timeoutMs) {
    dbgPrint("sendSMS to " + number);

    auto r = ensureTextMode();
    if (r.result != SIM800Result::SUCCESS) return r;

    auto r2 = _modem.sendCommand("AT+CMGS=\"" + number + "\"", 5000);
    if (r2.result == SIM800Result::PROMPT) {
        _modem.sendRaw(text + String((char)26));
        return _modem.waitForResponse(timeoutMs);
    }
    return r2;
}

bool extractAtField(const String& payload, const char* tag, String& out) {
    int idx = payload.indexOf(tag);
    if (idx < 0) return false;
    int colon = payload.indexOf(':', idx);
    if (colon < 0) return false;

    out = payload.substring(colon + 1);
    out.trim();
    return true;
}
