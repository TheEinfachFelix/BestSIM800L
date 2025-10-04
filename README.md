# BestSIM800L

This Lib actually cares about your Sim800L

## Comands

``` cpp

void enableDebug(Stream& debugOut = Serial);
void disableDebug();

SIM800Response testAT(uint32_t timeoutMs = 2000);

SIM800Response setPin(const String& pin, uint32_t timeoutMs = 2000, const String& pin2 = "", const String& puk= "", const String& puk2= "");
SIM800PinStatus getPinStatus(uint32_t timeoutMs = 2000);


SIM800Response getSignalQualityRaw(uint32_t timeoutMs = 2000);
bool getSignalQuality(CSQInfo& out, uint32_t timeoutMs = 2000);

SIM800Response getNetworkRegistrationRaw(uint32_t timeoutMs = 2000);
bool getNetworkRegistration(CREGInfo& out, uint32_t timeoutMs = 2000);

SIM800Response sendSMS(const String& number, const String& text, uint32_t timeoutMs = 10000);

```
