#include "Sensors/GasSensor.h"

GasSensor::GasSensor(uint8_t mq6AoPin, uint8_t mq7AoPin, uint8_t mq6DoPin, uint8_t mq7DoPin)
    : _mq6AoPin(mq6AoPin), _mq7AoPin(mq7AoPin), _mq6DoPin(mq6DoPin), _mq7DoPin(mq7DoPin),
      _mq6Base(0.0f), _mq7Base(0.0f) {}

void GasSensor::begin() {
  pinMode(_mq6DoPin, INPUT);
  pinMode(_mq7DoPin, INPUT);
  // Note: Analog pins (ADC1) do not require pinMode setting in ESP32 Arduino Core,
  // but it is safe to leave it implicitly configured during analogRead().
}

void GasSensor::calibrate(uint8_t iterations, uint32_t delayMs) {
  float mq6Sum = 0.0f;
  float mq7Sum = 0.0f;
  
  for (uint8_t i = 0; i < iterations; i++) {
    mq6Sum += readAverageVoltage(_mq6AoPin);
    mq7Sum += readAverageVoltage(_mq7AoPin);
    if (i < iterations - 1) {
      delay(delayMs);
    }
  }
  
  _mq6Base = mq6Sum / (float)iterations;
  _mq7Base = mq7Sum / (float)iterations;
}

float GasSensor::readMq6Voltage() const {
  return readAverageVoltage(_mq6AoPin);
}

float GasSensor::readMq7Voltage() const {
  return readAverageVoltage(_mq7AoPin);
}

bool GasSensor::isMq6DigitalActive() const {
  return (digitalRead(_mq6DoPin) == LOW);
}

bool GasSensor::isMq7DigitalActive() const {
  return (digitalRead(_mq7DoPin) == LOW);
}

bool GasSensor::checkDanger(float threshold) const {
  // Danger is triggered either by digital pin activation (low)
  // or analog voltage exceeding the calibrated baseline by the threshold value.
  bool mq6Danger = isMq6DigitalActive() || (readMq6Voltage() > _mq6Base + threshold);
  bool mq7Danger = isMq7DigitalActive() || (readMq7Voltage() > _mq7Base + threshold);
  
  return (mq6Danger || mq7Danger);
}

float GasSensor::readAverageVoltage(uint8_t pin) const {
  uint32_t total = 0;
  const uint8_t sampleCount = 15;
  
  for (uint8_t i = 0; i < sampleCount; i++) {
    total += analogRead(pin);
    delayMicroseconds(50);
  }
  
  // 12-bit ADC converts to 0-4095 range. Map to ESP32 standard 3.3V reference.
  return (float)(total / (float)sampleCount) * (3.3f / 4095.0f);
}
