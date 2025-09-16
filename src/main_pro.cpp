//
// Created by Isobek on 9/17/2025.
//
#include <Arduino.h>
#include <DHT.h> // DHT sensori uchun kutubxona

//=============================================================================
// === LOYIHA SOZLAMALARI (Professional Variant) ===
//=============================================================================
// 1. PINLARNI BELGILASH
#define MQ136_ANALOG_PIN 34
#define DHT_SENSOR_PIN 2
#define DHT_SENSOR_TYPE DHT11

// 2. FIZIK PARAMETRLAR
#define LOAD_RESISTOR_VALUE 5000.0f

// !!! ENG MUHIM QATOR: KALIBRATSIYA QIYMATI !!!
// Bu qiymat taxminan 20°C harorat va 65% namlikda olingani ma'qul.
#define R0_CALIBRATION_VALUE 18000.0f

// 3. FOIZ SOZLAMALARI
#define RATIO_FOR_100_PERCENT 0.3f

//=============================================================================
// === ASOSIY DASTUR LOGIKASI (o'zgartirish shart emas) ===
//=============================================================================

DHT dht(DHT_SENSOR_PIN, DHT_SENSOR_TYPE);

float mapf(float x, float in_min, float in_max, float out_min, float out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

float getSmoothedSensorValue(int pin, int samples) {
    long totalValue = 0;
    for (int i = 0; i < samples; i++) {
        totalValue += analogRead(pin);
        delay(5);
    }
    return static_cast<float>(totalValue) / samples;
}

void setup() {
    Serial.begin(9600);
    dht.begin();
    Serial.println("\n\n================================================");
    Serial.println("=== Ifloslanish Monitoringi (Professional Variant) ===");
    Serial.println("================================================");
    Serial.println("Datchik barqarorlashishi uchun 20-30 daqiqa kuting...");
    Serial.println();
}

void loop() {
    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();
    float r0_compensated = R0_CALIBRATION_VALUE;

    if (!isnan(temperature) && !isnan(humidity)) {
        r0_compensated = R0_CALIBRATION_VALUE * (1.0f - 0.02f * (temperature - 20.0f) - 0.01f * (humidity - 65.0f));
    } else {
        Serial.println("Ogohlantirish: DHT sensoridan ma'lumot olinmadi. Aniqlik pasayishi mumkin.");
    }

    float sensorRawValue = getSmoothedSensorValue(MQ136_ANALOG_PIN, 25);
    float sensorVoltage = sensorRawValue * (3.3f / 4095.0f);
    float RS_gas = ((3.3f * LOAD_RESISTOR_VALUE) / sensorVoltage) - LOAD_RESISTOR_VALUE;
    if (RS_gas < 0) RS_gas = 0;

    float ratio = RS_gas / r0_compensated;
    float pollution_percent = mapf(ratio, 1.0f, RATIO_FOR_100_PERCENT, 0.0f, 100.0f);
    pollution_percent = constrain(pollution_percent, 0.0f, 100.0f);

    if (!isnan(temperature)) {
       Serial.print("Harorat: "); Serial.print(temperature, 1); Serial.print("°C | ");
    }
    Serial.print("Ifloslanish darajasi: ");
    Serial.print(pollution_percent, 2);
    Serial.println(" %");

    delay(5000);
}