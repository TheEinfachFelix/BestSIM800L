#include "SIM800.hpp"

SIM800::SIM800() 
    : _serial(nullptr), _rxBuffer(), _inCommand(false),
      _debugEnabled(false), _debugOut(nullptr) {}

void SIM800::begin(HardwareSerial& serial, uint32_t baud) {
    _serial = &serial;
    _serial->begin(baud);
}

void SIM800::enableDebug(Stream& debugOut) {
    _debugEnabled = true;
    _debugOut = &debugOut;
}

void SIM800::disableDebug() {
    _debugEnabled = false;
    _debugOut = nullptr;
}

void SIM800::dbgPrint(const String& msg) {
    if (_debugEnabled && _debugOut) {
        _debugOut->println(msg);
    }
}

bool SIM800::readNextToken(String &outLine, bool &isPrompt, uint32_t timeoutMs) {
    outLine = "";
    isPrompt = false;
    if (!_serial) return false;

    uint32_t start = millis();
    while (millis() - start < timeoutMs) {
        while (_serial->available()) {
            char ch = (char)_serial->read();
            if (_debugEnabled && _debugOut) _debugOut->write(ch);

            if (ch == '>') {
                isPrompt = true;
                while (_serial->available()) {
                    char nxt = (char)_serial->peek();
                    if (nxt == ' ') { _serial->read(); if(_debugEnabled) _debugOut->write(nxt);} else break;
                }
                return true;
            }
            if (ch == '\r') continue;
            if (ch == '\n') {
                if (outLine.length() == 0) continue;
                isPrompt = false;
                return true;
            }
            outLine += ch;
            if (outLine.length() > 1024) return true;
        }
        delay(1);
    }
    return false;
}

void SIM800::dispatchEventIfMatched(const String& line) {
    for (auto &kv : _eventHandlers) {
        if (kv.first.length() > 0 && line.startsWith(kv.first)) {
            kv.second(line);
            return;
        }
    }
}

SIM800Response SIM800::sendCommand(const String& cmd, uint32_t timeoutMs) {
    if (!_serial) return { SIM800Result::UNKNOWN, "NO_SERIAL" };
    if (_inCommand) return { SIM800Result::UNKNOWN, "BUSY" };
    _inCommand = true;

    while (_serial->available()) { (void)_serial->read(); }

    dbgPrint(">> " + cmd);
    _serial->println(cmd);

    uint32_t start = millis();
    String collected = "";
    bool seenEcho = false;

    while (millis() - start < timeoutMs) {
        String line;
        bool isPrompt = false;
        if (!readNextToken(line, isPrompt, 200)) continue;

        if (isPrompt) {
            dbgPrint("<< PROMPT");
            _inCommand = false;
            return { SIM800Result::PROMPT, collected };
        }

        if (line.length() == 0) continue;
        if (!seenEcho && line == cmd) { seenEcho = true; continue; }

        if (line == "OK") {
            dbgPrint("<< OK");
            _inCommand = false;
            return { SIM800Result::SUCCESS, collected };
        }
        if (line == "ERROR") {
            dbgPrint("<< ERROR");
            _inCommand = false;
            return { SIM800Result::ERROR, collected };
        }

        bool handled = false;
        for (auto &kv : _eventHandlers) {
            if (kv.first.length() > 0 && line.startsWith(kv.first)) {
                kv.second(line);
                handled = true;
                break;
            }
        }
        if (handled) continue;

        if (collected.length()) collected += '\n';
        collected += line;
    }

    dbgPrint("<< TIMEOUT");
    _inCommand = false;
    return { SIM800Result::TIMEOUT, collected };
}

void SIM800::sendRaw(const String& data) {
    if (!_serial) return;
    dbgPrint(">> RAW: " + data);
    _serial->print(data);
}

SIM800Response SIM800::waitForResponse(uint32_t timeoutMs) {
    if (!_serial) return { SIM800Result::UNKNOWN, "NO_SERIAL" };
    if (_inCommand) return { SIM800Result::UNKNOWN, "BUSY" };

    _inCommand = true;
    uint32_t start = millis();
    String collected = "";

    while (millis() - start < timeoutMs) {
        String line;
        bool isPrompt = false;
        if (!readNextToken(line, isPrompt, 200)) continue;

        if (isPrompt) {
            dbgPrint("<< PROMPT");
            _inCommand = false;
            return { SIM800Result::PROMPT, collected };
        }
        if (line.length() == 0) continue;

        if (line == "OK") {
            dbgPrint("<< OK");
            _inCommand = false;
            return { SIM800Result::SUCCESS, collected };
        }
        if (line == "ERROR") {
            dbgPrint("<< ERROR");
            _inCommand = false;
            return { SIM800Result::ERROR, collected };
        }

        bool handled = false;
        for (auto &kv : _eventHandlers) {
            if (kv.first.length() > 0 && line.startsWith(kv.first)) {
                kv.second(line);
                handled = true;
                break;
            }
        }
        if (handled) continue;

        if (collected.length()) collected += '\n';
        collected += line;
    }

    dbgPrint("<< TIMEOUT");
    _inCommand = false;
    return { SIM800Result::TIMEOUT, collected };
}

void SIM800::onEvent(const String& prefix, EventHandler cb) {
    _eventHandlers[prefix] = cb;
}

void SIM800::loop() {
    if (!_serial) return;
    if (_inCommand) return;

    while (_serial->available()) {
        char ch = (char)_serial->read();
        if (_debugEnabled && _debugOut) _debugOut->write(ch);

        if (ch == '\r') continue;
        if (ch == '\n') {
            if (_rxBuffer.length() > 0) {
                dispatchEventIfMatched(_rxBuffer);
                _rxBuffer = "";
            }
        } else {
            if (ch == '>') {
                String tmp = ">";
                dispatchEventIfMatched(tmp);
                continue;
            }
            _rxBuffer += ch;
            if (_rxBuffer.length() > 1024) _rxBuffer = "";
        }
    }
}
