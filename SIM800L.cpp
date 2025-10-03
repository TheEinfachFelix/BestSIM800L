#include "SIM800L.hpp"

SIM800::SIM800() : _serial(nullptr) {}

void SIM800::begin(HardwareSerial& serial, uint32_t baud) {
    _serial = &serial;
    _serial->begin(baud);
    _rxBuffer.reserve(128);
}

void SIM800::sendCommand(const String& cmd, Callback cb, uint32_t timeoutMs) {
    Command c{cmd, cb, timeoutMs, millis()};
    _commandQueue.push(c);
    _serial->println(cmd);
}

void SIM800::onEvent(const String& eventName, Callback cb) {
    _eventHandlers[eventName] = cb;
}

void SIM800::loop() {
    // Eingehende Bytes lesen
    while (_serial && _serial->available()) {
        char ch = _serial->read();
        if (ch == '\r') continue;
        if (ch == '\n') {
            if (_rxBuffer.length() > 0) {
                handleLine(_rxBuffer);
                _rxBuffer = "";
            }
        } else {
            _rxBuffer += ch;
        }
    }

    checkTimeouts();
}

void SIM800::handleLine(const String& line) {
    if (!_commandQueue.empty()) {
        auto& cmd = _commandQueue.front();

        if (line == "OK" || line == "ERROR") {
            if (cmd.cb) cmd.cb(line);
            _commandQueue.pop();
            return;
        }
    }

    // Prüfen ob Event
    if (_eventHandlers.count(line)) {
        _eventHandlers[line](line);
    }
}

void SIM800::checkTimeouts() {
    if (_commandQueue.empty()) return;

    auto& cmd = _commandQueue.front();
    if (millis() - cmd.startTime > cmd.timeout) {
        if (cmd.cb) cmd.cb("TIMEOUT");
        _commandQueue.pop();
    }
}
