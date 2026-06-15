#include "Sensors/FireSensor.h"

FireSensor::FireSensor(uint8_t digitalPin) : _pin(digitalPin) {}

void FireSensor::begin() {
  pinMode(_pin, INPUT_PULLUP); // Ruxsat etilgan (ichki pull-up bor)
}

bool FireSensor::isFireDetected() const {
  return (digitalRead(_pin) == LOW); // LOW indicates fire detected by flame sensor
}
