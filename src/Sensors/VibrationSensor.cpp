#include "Sensors/VibrationSensor.h"

VibrationSensor::VibrationSensor(uint8_t sdaPin, uint8_t sclPin, uint8_t i2cAddress)
    : _sdaPin(sdaPin), _sclPin(sclPin), _address(i2cAddress), _i2c(1), _active(false),
      _lastX(0.0f), _lastY(0.0f), _lastZ(0.0f), _currentDelta(0.0f) {}

bool VibrationSensor::begin() {
  _i2c.begin(_sdaPin, _sclPin, 100000); // 100kHz (Stable mode)
  _i2c.setTimeOut(2000); // 2 second timeout
  delay(100);
  
  _i2c.beginTransmission(_address);
  _i2c.write(0x6B); // Power Management register
  _i2c.write(0);    // Wake up command
  
  if (_i2c.endTransmission(true) == 0) {
    _active = true;
    return true;
  }
  return false;
}

void VibrationSensor::update() {
  if (!_active) return;
  
  _i2c.beginTransmission(_address);
  _i2c.write(0x3B); // Accel X High register
  _i2c.endTransmission(false);
  _i2c.requestFrom((uint8_t)_address, (size_t)6, true);

  if (_i2c.available() < 6) return;

  int16_t x = (int16_t)(_i2c.read() << 8 | _i2c.read());
  int16_t y = (int16_t)(_i2c.read() << 8 | _i2c.read());
  int16_t z = (int16_t)(_i2c.read() << 8 | _i2c.read());

  // Convert raw values to G-force (16384 LSB/g at default +/- 2g range)
  float ax = (float)x / 16384.0f;
  float ay = (float)y / 16384.0f;
  float az = (float)z / 16384.0f;

  // Calculate change in G-force since the last reading
  _currentDelta = abs(ax - _lastX) + abs(ay - _lastY) + abs(az - _lastZ);
  
  _lastX = ax; 
  _lastY = ay; 
  _lastZ = az;
}

bool VibrationSensor::isQuake(float threshold) const {
  if (!_active) return false;
  return (_currentDelta > threshold);
}

float VibrationSensor::getTemperature() {
  if (!_active) return 0.0f;
  
  _i2c.beginTransmission(_address);
  _i2c.write(0x41); // Temp Out High register
  _i2c.endTransmission(false);
  _i2c.requestFrom((uint8_t)_address, (size_t)2, true);
  
  if (_i2c.available() == 2) {
    int16_t t = (int16_t)(_i2c.read() << 8 | _i2c.read());
    // MPU6050 temperature conversion formula from datasheet
    return (float)(t / 340.0f) + 36.53f;
  }
  return 0.0f;
}
