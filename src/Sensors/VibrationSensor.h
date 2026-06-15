#ifndef VIBRATION_SENSOR_H
#define VIBRATION_SENSOR_H

#include <Arduino.h>
#include <Wire.h>

class VibrationSensor {
public:
    VibrationSensor(uint8_t sdaPin, uint8_t sclPin, uint8_t i2cAddress = 0x68);
    
    bool begin();
    void update();
    bool isQuake(float threshold = 0.6f) const;
    float getTemperature();
    bool isActive() const { return _active; }

private:
    uint8_t _sdaPin;
    uint8_t _sclPin;
    uint8_t _address;
    TwoWire _i2c;
    bool _active;
    
    float _lastX;
    float _lastY;
    float _lastZ;
    float _currentDelta;
};

#endif // VIBRATION_SENSOR_H
