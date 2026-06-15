#ifndef FIRE_SENSOR_H
#define FIRE_SENSOR_H

#include <Arduino.h>

class FireSensor {
public:
    FireSensor(uint8_t digitalPin);
    
    void begin();
    bool isFireDetected() const;

private:
    uint8_t _pin;
};

#endif // FIRE_SENSOR_H
