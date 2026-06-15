#ifndef TELEMETRY_H
#define TELEMETRY_H

#include <Arduino.h>

class Telemetry {
public:
    Telemetry(const char* ssid, const char* password, const char* serverUrl, const char* token);
    
    void begin();
    void handleConnection();
    bool isConnected() const;
    
    bool sendPayload(bool alarm, bool fire, bool quake, float mq6, float mq7, float temp);

private:
    const char* _ssid;
    const char* _password;
    const char* _serverUrl;
    const char* _token;
    unsigned long _lastConnectionCheck;
};

#endif // TELEMETRY_H
