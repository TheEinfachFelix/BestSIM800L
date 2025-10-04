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
    void begin(HardwareSerial& serial, uint32_t baud = 9600, int dtrPin = -1, int rtsPin = -1);

    SIM800Response sendCommand(const String& cmd, uint32_t timeoutMs = 2000);
    void sendRaw(const String& data);
    SIM800Response waitForResponse(uint32_t timeoutMs = 5000);

    void onEvent(const String& prefix, EventHandler cb);
    void loop();

    // Debug aktivieren (optional andere Serial als Ziel)
    void enableDebug(Stream& debugOut = Serial);
    void disableDebug();

    void reset();
    void toggleDtr();

private:
    HardwareSerial* _serial;
    String _rxBuffer;
    std::map<String, EventHandler> _eventHandlers;
    bool _inCommand;

    bool _debugEnabled;
    Stream* _debugOut;

    int dtrPin = -1;
    int rtsPin = -1;

    bool readNextToken(String &outLine, bool &isPrompt, uint32_t timeoutMs);
    void dispatchEventIfMatched(const String& line);

    void dbgPrint(const String& msg);
    void dbgByte(char c);
};
