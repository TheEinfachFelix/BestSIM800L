#pragma once
#include <Arduino.h>
#include <functional>
#include <map>

enum class SIM800Result {
    SUCCESS,
    ERROR,
    TIMEOUT,
    PROMPT,
    UNKNOWN
};

struct SIM800Response {
    SIM800Result result;
    String payload; // gesammelte Antwortzeilen (ohne echo)
};

class SIM800 {
public:
    using EventHandler = std::function<void(const String&)>;

    SIM800();
    void begin(HardwareSerial& serial, uint32_t baud = 9600);

    // Blockierendes Senden eines AT-Commands. Liefert Ergebnis + gesammelte Antwort.
    SIM800Response sendCommand(const String& cmd, uint32_t timeoutMs = 2000);

    // Roh senden (z.B. SMS-Text + Ctrl-Z)
    void sendRaw(const String& data);

    // Warten auf das finale OK/ERROR (z.B. nach sendRaw)
    SIM800Response waitForResponse(uint32_t timeoutMs = 5000);

    // Events abonnieren: prefix wird mit line.startsWith(prefix) verglichen
    void onEvent(const String& prefix, EventHandler cb);

    // Nicht-blockierender Loop: kann alternativ statt sendCommand verwendet werden
    void loop();

private:
    HardwareSerial* _serial;
    String _rxBuffer;
    std::map<String, EventHandler> _eventHandlers;
    bool _inCommand;

    // Liest bis \n oder bis Prompt '>' erscheint. Gibt true, wenn ein Token empfangen wurde.
    // outLine = gelesene Zeile (ohne CR/LF), isPrompt = true falls '>' gefunden wurde.
    bool readNextToken(String &outLine, bool &isPrompt, uint32_t timeoutMs);

    // Interne Event-Dispatcher für eingehende Lines
    void dispatchEventIfMatched(const String& line);
};
