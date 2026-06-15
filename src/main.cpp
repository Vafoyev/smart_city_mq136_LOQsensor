#include <Arduino.h>
#include <esp_task_wdt.h>
#include "Config.h"
#include "Sensors/GasSensor.h"
#include "Sensors/VibrationSensor.h"
#include "Sensors/FireSensor.h"
#include "Network/Telemetry.h"

// --- SYSTEM MODULES ---
GasSensor gasSensor(PIN_MQ6_AO, PIN_MQ7_AO, PIN_MQ6_DO, PIN_MQ7_DO);
VibrationSensor vibrationSensor(I2C_MPU_SDA, I2C_MPU_SCL);
FireSensor fireSensor(PIN_FIRE_DO);
Telemetry telemetry(WIFI_SSID, WIFI_PASS, SERVER_URL, TELEMETRY_TOKEN);

// --- SCHEDULING VARIABLES ---
unsigned long lastSentTime = 0;

void setup() {
  Serial.begin(115200);
  delay(1000); // Allow serial port to initialize
  
  Serial.println("\n\n=========================================");
  Serial.println("=== DECENTRALIZED SMART CITY SENSOR  ===");
  Serial.println("=========================================");

  // Initialize Watchdog Timer for system safety
  esp_task_wdt_init(WDT_TIMEOUT, true);
  esp_task_wdt_add(nullptr);

  // Initialize sensors and outputs
  pinMode(PIN_BUZZER, OUTPUT);
  
  // Quick startup beep (150ms at 2000Hz) to verify buzzer hardware is working
  tone(PIN_BUZZER, 2000);
  delay(150);
  noTone(PIN_BUZZER);

  fireSensor.begin();
  gasSensor.begin();
  
  if (vibrationSensor.begin()) {
    Serial.println("[INIT] MPU6050 accelerometer initialized successfully");
  } else {
    Serial.println("[WARNING] MPU6050 accelerometer initialization failed");
  }

  // Calibration Phase
  Serial.println("[INIT] Starting sensor baseline calibration...");
  for (int i = 0; i < 30; i++) {
    gasSensor.calibrate(1, 100); // Calibrate 1 iteration, then wait 100ms
    vibrationSensor.update();     // Read initial vibrations to update baseline values
    esp_task_wdt_reset();         // Keep watchdog happy
  }
  
  Serial.printf("[INFO] Base voltages calibrated - MQ6: %.2fv, MQ7: %.2fv\n", 
                gasSensor.getMq6Base(), gasSensor.getMq7Base());

  // Connect to Wi-Fi
  Serial.println("[INIT] Connecting to WiFi...");
  telemetry.begin();
  
  // Non-blocking WiFi connection wait (up to 5 seconds)
  unsigned long startWiFiWait = millis();
  while (!telemetry.isConnected() && (millis() - startWiFiWait < 5000)) {
    delay(100);
    esp_task_wdt_reset();
    Serial.print(".");
  }
  Serial.println();
  
  if (telemetry.isConnected()) {
    Serial.println("[SUCCESS] WiFi connected successfully");
    
    // Send initial safe-state (alarm: false) packet immediately upon boot
    Serial.println("[INIT] Sending initial alarm-off (false) packet to backend...");
    float mq6Volts = gasSensor.readMq6Voltage();
    float mq7Volts = gasSensor.readMq7Voltage();
    float temp = vibrationSensor.getTemperature();
    
    telemetry.sendPayload(false, false, false, mq6Volts, mq7Volts, temp);
    lastSentTime = millis(); // Set timer base to prevent immediate re-sending in loop
  } else {
    Serial.println("[WARNING] WiFi connection failed (operating offline)");
  }
}

void loop() {
  esp_task_wdt_reset(); // Feed the watchdog

  // 1. Maintain WiFi connection in a non-blocking rate-limited way
  telemetry.handleConnection();

  // 2. Poll and update sensors
  vibrationSensor.update();
  
  float mq6Volts = gasSensor.readMq6Voltage();
  float mq7Volts = gasSensor.readMq7Voltage();
  float temp = vibrationSensor.getTemperature();
  bool fire = fireSensor.isFireDetected();
  bool quake = vibrationSensor.isQuake(0.6f);
  
  // 3. Evaluate hazard status
  bool gasDanger = gasSensor.checkDanger(SENS_THRESH);
  bool alarm = fire || gasDanger || quake;

  // Control the buzzer (ON at 2000Hz during alarm, OFF during normal state)
  // Using tone() ensures both passive and active buzzers can produce audible sound
  if (alarm) {
    tone(PIN_BUZZER, 2000);
  } else {
    noTone(PIN_BUZZER);
  }

  // 4. Send telemetry payload to server
  // Send immediately if the alarm status changes to report state changes immediately (on/off).
  // Otherwise, send periodically (every 3 seconds during alarm, every 60 seconds during normal state).
  static bool lastAlarm = false;
  bool alarmChanged = (alarm != lastAlarm);
  unsigned long interval = alarm ? 3000 : 60000;
  
  if (alarmChanged || (millis() - lastSentTime > interval)) {
    lastSentTime = millis();
    lastAlarm = alarm;
    telemetry.sendPayload(alarm, fire, quake, mq6Volts, mq7Volts, temp);
    
    // Debug output
    Serial.printf("Telemetry Sent (Trigger: %s) - Alarm: %s, MQ6:%.2fv, MQ7:%.2fv, Temp:%.1fC, Fire:%d, Quake:%d, WiFi:%d\n", 
                  alarmChanged ? "State Change" : "Interval",
                  alarm ? "true" : "false",
                  mq6Volts, mq7Volts, temp, fire, quake, telemetry.isConnected());
  }

  delay(10); // Relieve CPU stress
}
