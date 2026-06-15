#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// --- WiFi & SERVER CONFIG ---
extern const char* WIFI_SSID;
extern const char* WIFI_PASS;
extern const char* SERVER_URL;
extern const char* TELEMETRY_TOKEN;

// --- PIN CONFIGURATION ---
// Analog Pins (Only 32, 33, 34, 35, 36, 39 are valid ADC1 pins on ESP32, which work with WiFi active)
#define PIN_MQ6_AO   34  // LPG Analog Input
#define PIN_MQ7_AO   32  // CO Analog Input
#define PIN_FIRE_DO  14  // Flame Digital Input (Internal pull-up enabled)

// Digital Pins
#define PIN_MQ6_DO   26  // MQ6 Digital Input (LPG threshold indicator)
#define PIN_MQ7_DO   27  // MQ7 Digital Input (CO threshold indicator)
#define PIN_BUZZER   25  // Buzzer Output Pin (Active high)

// --- I2C CONFIGURATION ---
#define I2C_MPU_SDA  19  // MPU6050 SDA
#define I2C_MPU_SCL  18  // MPU6050 SCL

// --- SYSTEM SETTINGS ---
#define WDT_TIMEOUT   15   // Watchdog timeout in seconds
#define SENS_THRESH   0.5f // Sensitivity voltage threshold above calibrated base
#define WIFI_CHECK_INTERVAL 10000 // Check Wi-Fi state every 10 seconds

#endif // CONFIG_H
