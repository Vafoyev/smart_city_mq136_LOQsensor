#include <Arduino.h>

//=============================================================================
// === LOYIHA SOZLAMALARI (Sizning Qurilmangiz Uchun Moslashtirilgan) ===
//=============================================================================

// 1. PINNI BELGILASH
#define MQ136_ANALOG_PIN 34   // Signalni GPIO34 ga uladingiz, shunday qoldiring.

// 2. FIZIK PARAMETRLAR
// Siz 10kΩ rezistor ishlatdingiz, shuning uchun bu qiymatni 10000.0f ga o'zgartirdik.
#define LOAD_RESISTOR_VALUE 10000.0f

// !!! ENG MUHIM QATOR: KALIBRATSIYA QIYMATI !!!
// Bu qiymatni albatta o'zingiz toza havoda kalibrlab, aniq qiymatini shu yerga yozishingiz kerak!
// Dastlabki urinish uchun 18000.0f turibdi.
#define R0_CALIBRATION_VALUE 7300.0f

// 3. FOIZ SOZLAMALARI (Sezgirlikni boshqaradi)
// Bu son qancha kichik bo'lsa, qurilma shuncha sezgir bo'ladi.
#define RATIO_FOR_100_PERCENT 0.3f

//=============================================================================
// === ASOSIY DASTUR LOGIKASI (bu qismiga tegish shart emas) ===
//=============================================================================

// Sonli qiymatlarni bir diapazondan boshqasiga o'tkazish uchun yordamchi funksiya
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
    Serial.begin(9600); // Agar platformio.ini da 115200 bo'lsa, buni ham 115200 qiling.

    Serial.println("\n\n==================================================");
    Serial.println("=== Ifloslanish Monitoringi (Asosiy Versiya) ===");
    Serial.println("==================================================");
    Serial.println("Datchik barqarorlashishi uchun 20-30 daqiqa kuting...");
    Serial.println();
}

void loop() {
    // 1-QADAM: MQ-136 dan ma'lumotni o'qish va qarshilikni (RS) hisoblash
    float sensorRawValue = getSmoothedSensorValue(MQ136_ANALOG_PIN, 25);
    float sensorVoltage = sensorRawValue * (3.3f / 4095.0f); // ESP32 ADC: 12 bit, 3.3V
    float RS_gas = ((3.3f * LOAD_RESISTOR_VALUE) / sensorVoltage) - LOAD_RESISTOR_VALUE;
    if (RS_gas < 0) RS_gas = 0;

    // 2-QADAM: Nisbatni hisoblash va uni foizga aylantirish
    // Bu yerda biz o'zgarmas, kalibrlangan R0 ni ishlatamiz.
    float ratio = RS_gas / R0_CALIBRATION_VALUE;

    float pollution_percent = mapf(ratio, 1.0f, RATIO_FOR_100_PERCENT, 0.0f, 100.0f);
    pollution_percent = constrain(pollution_percent, 0.0f, 100.0f); // Natijani 0-100 oralig'ida cheklaymiz

    // 3-QADAM: Natijani Serial Monitorga chiqarish
    Serial.print("Ifloslanish darajasi: ");
    Serial.print(pollution_percent, 2); // Foizni ikkita o'nlik xona aniqligida chiqaramiz
    Serial.println(" %");

    delay(5000); // Har 5 soniyada yangi ma'lumot olamiz
}