#pragma once
#include <Arduino.h>
#include <functional>
#include <queue>
#include <map>

class SIM800 {
public:
    using Callback = std::function<void(const String&)>;

    SIM800();
    void begin(HardwareSerial& serial, uint32_t baud = 9600);

    // Befehl senden mit Callback
    void sendCommand(const String& cmd, Callback cb, uint32_t timeoutMs = 1000);

    // Events abonnieren (z.B. "RING", "SMS Ready", "ERROR")
    void onEvent(const String& eventName, Callback cb);

    // im Loop aufrufen
    void loop();

private:
    struct Command {
        String cmd;
        Callback cb;
        uint32_t timeout;
        uint32_t startTime;
    };

    HardwareSerial* _serial;
    std::queue<Command> _commandQueue;
    std::map<String, Callback> _eventHandlers;

    String _rxBuffer;

    void handleLine(const String& line);
    void checkTimeouts();
};
