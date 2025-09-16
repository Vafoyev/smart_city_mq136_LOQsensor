#include <Arduino.h>

//=============================================================================
// === LOYIHA SOZLAMALARI (Asosiy Variant) ===
//=============================================================================
// 1. PINNI BELGILASH
#define MQ136_ANALOG_PIN 34   // MQ-136 ning A0 pini ulangan GPIO

// 2. FIZIK PARAMETRLAR
#define LOAD_RESISTOR_VALUE 5000.0f // Modul platasidagi yuklama rezistorining qiymati (Ohmda).

// !!! ENG MUHIM QATOR: KALIBRATSIYA QIYMATI !!!
// Bu qiymatni albatta o'zingiz toza havoda kalibrlab, aniq qiymatini shu yerga yozishingiz kerak!
#define R0_CALIBRATION_VALUE 18000.0f

// 3. FOIZ SOZLAMALARI (Sezgirlikni boshqaradi)
#define RATIO_FOR_100_PERCENT 0.3f

//=============================================================================
// === ASOSIY DASTUR LOGIKASI (o'zgartirish shart emas) ===
//=============================================================================

// Sonli qiymatlarni bir diapazondan boshqasiga o'tkazish uchun funksiya
float mapf(float x, float in_min, float in_max, float out_min, float out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

// Sensor qiymatini silliqlash (optimallashtirish) uchun funksiya
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
    Serial.println("\n\n==============================================");
    Serial.println("=== Ifloslanish Monitoringi (Asosiy Variant) ===");
    Serial.println("==============================================");
    Serial.println("Datchik barqarorlashishi uchun 20-30 daqiqa kuting...");
    Serial.println();
}

void loop() {
    // 1-QADAM: MQ-136 dan ma'lumotni o'qish va qarshilikni (RS) hisoblash
    float sensorRawValue = getSmoothedSensorValue(MQ136_ANALOG_PIN, 25);
    float sensorVoltage = sensorRawValue * (3.3f / 4095.0f);
    float RS_gas = ((3.3f * LOAD_RESISTOR_VALUE) / sensorVoltage) - LOAD_RESISTOR_VALUE;
    if (RS_gas < 0) RS_gas = 0;

    // 2-QADAM: Nisbatni hisoblash va uni foizga aylantirish
    float ratio = RS_gas / R0_CALIBRATION_VALUE;
    float pollution_percent = mapf(ratio, 1.0f, RATIO_FOR_100_PERCENT, 0.0f, 100.0f);
    pollution_percent = constrain(pollution_percent, 0.0f, 100.0f);

    // 3-QADAM: Natijani Serial Monitorga chiqarish
    Serial.print("Ifloslanish darajasi: ");
    Serial.print(pollution_percent, 2);
    Serial.println(" %");

    delay(5000);
}