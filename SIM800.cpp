#include "SIM800.hpp"

SIM800::SIM800() : _serial(nullptr), _rxBuffer(), _inCommand(false) {}

void SIM800::begin(HardwareSerial& serial, uint32_t baud) {
    _serial = &serial;
    _serial->begin(baud);
}

// Hilfsfunktion: liest bis '\n' oder bis '>' (Prompt). Timeouts in kleinen Schritten.
bool SIM800::readNextToken(String &outLine, bool &isPrompt, uint32_t timeoutMs) {
    outLine = "";
    isPrompt = false;
    if (!_serial) return false;

    uint32_t start = millis();
    while (millis() - start < timeoutMs) {
        while (_serial->available()) {
            char ch = (char)_serial->read();
            // Prompt detection: '>' (oft ohne NL)
            if (ch == '>') {
                isPrompt = true;
                // discard trailing space if any
                while (_serial->available()) {
                    char nxt = (char)_serial->peek();
                    if (nxt == ' ') { _serial->read(); } else break;
                }
                return true;
            }
            if (ch == '\r') continue;
            if (ch == '\n') {
                // ignore empty lines
                if (outLine.length() == 0) continue;
                isPrompt = false;
                return true;
            }
            outLine += ch;
            // Protect against extremely long lines
            if (outLine.length() > 1024) {
                return true;
            }
        }
        delay(1); // yield; erlaubt auch ESP internals zu laufen
    }
    return false;
}

void SIM800::dispatchEventIfMatched(const String& line) {
    for (auto &kv : _eventHandlers) {
        const String &prefix = kv.first;
        if (prefix.length() > 0 && line.startsWith(prefix)) {
            kv.second(line);
            return;
        }
    }
}

// Blockierend, verarbeitet eingehende Zeilen bis OK/ERROR oder Prompt oder Timeout.
SIM800Response SIM800::sendCommand(const String& cmd, uint32_t timeoutMs) {
    if (!_serial) return { SIM800Result::UNKNOWN, "NO_SERIAL" };

    // einfache Reentrancy-Sperre
    if (_inCommand) return { SIM800Result::UNKNOWN, "BUSY" };
    _inCommand = true;

    // kleine Drain: vorhandene Bytes kurz leeren (verhindert alte Daten)
    uint32_t drainStart = millis();
    while (_serial->available() && (millis() - drainStart) < 50) { (void)_serial->read(); }

    // Senden
    _serial->print(cmd+"\r");

    uint32_t start = millis();
    String collected = "";
    bool seenEcho = false;

    while (millis() - start < timeoutMs) {
        String line;
        bool isPrompt = false;
        // Lese mit kleinem inner-loop timeout; so können Events schnell dispatched werden
        if (!readNextToken(line, isPrompt, 200)) {
            continue;
        }

        if (isPrompt) {
            // Prompt -> Modul erwartet jetzt Body (z.B. SMS). Rückgabe PROMPT, payload bisher gesammelt.
            _inCommand = false;
            return { SIM800Result::PROMPT, collected };
        }

        // ignore pure empty lines
        if (line.length() == 0) continue;

        // Modul echo: oft wird der gesendete Befehl wiederholt. Ignoriere das Echo.
        if (!seenEcho && line == cmd) {
            seenEcho = true;
            continue;
        }

        // Finalstatus prüfen
        if (line == "OK") {
            _inCommand = false;
            return { SIM800Result::SUCCESS, collected };
        }
        if (line == "ERROR") {
            _inCommand = false;
            return { SIM800Result::ERROR, collected };
        }

        // Falls die Line zu einem Event passt, dispatchen und nicht zur Response zählen
        bool handled = false;
        for (auto &kv : _eventHandlers) {
            if (kv.first.length() > 0 && line.startsWith(kv.first)) {
                kv.second(line);
                handled = true;
                break;
            }
        }
        if (handled) continue;

        // sonst: zur gesammelten Antwort hinzufügen
        if (collected.length()) collected += '\n';
        collected += line;
    }

    _inCommand = false;
    return { SIM800Result::TIMEOUT, collected };
}

void SIM800::sendRaw(const String& data) {
    if (!_serial) return;
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
            // unerwartetes prompt während waitForResponse - gib PROMPT zurück
            _inCommand = false;
            return { SIM800Result::PROMPT, collected };
        }

        if (line.length() == 0) continue;

        if (line == "OK") {
            _inCommand = false;
            return { SIM800Result::SUCCESS, collected };
        }
        if (line == "ERROR") {
            _inCommand = false;
            return { SIM800Result::ERROR, collected };
        }

        // Events dispatchen
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

    _inCommand = false;
    return { SIM800Result::TIMEOUT, collected };
}

void SIM800::onEvent(const String& prefix, EventHandler cb) {
    _eventHandlers[prefix] = cb;
}

// Nicht-blockierender loop: liest komplette Zeilen und dispatcht Events (wenn keine sendCommand läuft)
void SIM800::loop() {
    if (!_serial) return;
    if (_inCommand) return; // während eines blocking-commands nicht parallel arbeiten

    while (_serial->available()) {
        char ch = (char)_serial->read();
        if (ch == '\r') continue;
        if (ch == '\n') {
            if (_rxBuffer.length() > 0) {
                dispatchEventIfMatched(_rxBuffer);
                _rxBuffer = "";
            }
        } else {
            // prompt detection auch hier
            if (ch == '>') {
                String tmp = ">";
                dispatchEventIfMatched(tmp);
                continue;
            }
            _rxBuffer += ch;
            if (_rxBuffer.length() > 1024) _rxBuffer = ""; // safety
        }
    }
}
