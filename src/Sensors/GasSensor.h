#ifndef GAS_SENSOR_H
#define GAS_SENSOR_H

#include <Arduino.h>

class GasSensor {
public:
    GasSensor(uint8_t mq6AoPin, uint8_t mq7AoPin, uint8_t mq6DoPin, uint8_t mq7DoPin);
    
    void begin();
    void calibrate(uint8_t iterations = 30, uint32_t delayMs = 100);
    
    float readMq6Voltage() const;
    float readMq7Voltage() const;
    
    bool isMq6DigitalActive() const;
    bool isMq7DigitalActive() const;
    
    bool checkDanger(float threshold) const;
    
    float getMq6Base() const { return _mq6Base; }
    float getMq7Base() const { return _mq7Base; }

private:
    float readAverageVoltage(uint8_t pin) const;

    uint8_t _mq6AoPin;
    uint8_t _mq7AoPin;
    uint8_t _mq6DoPin;
    uint8_t _mq7DoPin;
    
    float _mq6Base;
    float _mq7Base;
};

#endif // GAS_SENSOR_H
