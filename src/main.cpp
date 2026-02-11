#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h> // HTTPS uchun
#include <esp_task_wdt.h> // Tizim "soqchisi"

// --- GOLD STANDARD CONFIGURATION ---

// 1. WIFI & SERVER (O'zingiznikiga o'zgartiring)
const char* WIFI_SSID = "Isobek's iPhone";
const char* WIFI_PASS = "12345678";
const char* SERVER_URL = "https://mchs.unusual.uz/api/sensor-data";

// 2. PIN CONFIGURATION (ESP32 ADC1 faqat WiFi bilan ishlaydi!)
// Analog Pinlar (Faqat 32, 33, 34, 35, 36, 39 ruxsat etiladi)
#define PIN_MQ6_AO   34  // LPG (Propan/Butan)
#define PIN_MQ9_AO   33  // Yonuvchi Gazlar
#define PIN_MQ7_AO   32  // CO (Is gazi)
#define PIN_FIRE_DO  35  // Olov Sensori (Analog/Digital sifatida ishlatish mumkin)

// Digital Alarm Pinlar (Ixtiyoriy GPIO)
#define PIN_MQ6_DO   26
#define PIN_MQ9_DO   14
#define PIN_MQ7_DO   27

// 3. I2C PINLARI (Ikki alohida liniya)
#define I2C_LCD_SDA  21
#define I2C_LCD_SCL  22
#define I2C_MPU_SDA  19
#define I2C_MPU_SCL  18

// 4. SOZLAMALAR
#define WDT_TIMEOUT  15  // 15 soniya (Xavfsizroq)
#define SENS_THRESH  0.5 // Sezgirlik (Kalibratsiyadan keyingi +V kuchlanish)

// --- OBYEKTLAR ---
// LCD 0x27 yoki 0x3F manzilida bo'ladi
LiquidCrystal_I2C lcd(0x27, 16, 4);
TwoWire I2C_MPU = TwoWire(1); // Ikkinchi I2C port

// --- GLOBAL O'ZGARUVCHILAR ---
float baseMq6 = 0, baseMq9 = 0, baseMq7 = 0;
float lastX, lastY, lastZ;
bool mpuActive = false;
unsigned long lastSentTime = 0;
unsigned long lastLcdTime = 0;

// --- DATCHIKNI QAYTA ISHLASH (10 ta namuna o'rtachasi) ---
float readAdcVoltage(int pin) {
  uint32_t total = 0;
  for (int i = 0; i < 15; i++) {
    total += analogRead(pin);
    delayMicroseconds(50);
  }
  // 12-bit ADC (0-4095) -> 3.3V convert
  return (float)(total / 15.0f) * (3.3f / 4095.0f);
}

// --- MPU6050 (ZILZILA) INIT ---
void initMPU() {
  I2C_MPU.begin(I2C_MPU_SDA, I2C_MPU_SCL, 100000); // 100kHz (Barqarorroq)
  I2C_MPU.setTimeOut(2000); // 2 soniya timeout
  delay(100);
  I2C_MPU.beginTransmission(0x68);
  I2C_MPU.write(0x6B); // Power Management
  I2C_MPU.write(0);    // Wake up
  if (I2C_MPU.endTransmission(true) == 0) {
    mpuActive = true;
    Serial.println("MPU6050: OK");
  } else {
    Serial.println("MPU6050: ERROR");
  }
}

// --- HARORATNI O'QISH ---
float getTemp() {
  if (!mpuActive) return 0.0f;
  I2C_MPU.beginTransmission(0x68);
  I2C_MPU.write(0x41);
  I2C_MPU.endTransmission(false);
  I2C_MPU.requestFrom((uint8_t)0x68, (size_t)2, true);
  if(I2C_MPU.available() == 2) {
    int16_t t = (int16_t)(I2C_MPU.read() << 8 | I2C_MPU.read());
    return (float)(t / 340.0f) + 36.53f;
  }
  return 0.0f;
}

// --- ZILZILA LOGIKASI ---
bool checkQuake() {
  if (!mpuActive) return false;
  I2C_MPU.beginTransmission(0x68);
  I2C_MPU.write(0x3B); // Accel X High
  I2C_MPU.endTransmission(false);
  I2C_MPU.requestFrom((uint8_t)0x68, (size_t)6, true);

  if (I2C_MPU.available() < 6) return false;

  int16_t x = (int16_t)(I2C_MPU.read() << 8 | I2C_MPU.read());
  int16_t y = (int16_t)(I2C_MPU.read() << 8 | I2C_MPU.read());
  int16_t z = (int16_t)(I2C_MPU.read() << 8 | I2C_MPU.read());

  // O'lchov birligi (G-forcega yaqin nisbat)
  float ax = (float)x / 16384.0f;
  float ay = (float)y / 16384.0f;
  float az = (float)z / 16384.0f;

  float delta = abs(ax - lastX) + abs(ay - lastY) + abs(az - lastZ);
  lastX = ax; lastY = ay; lastZ = az;

  // 0.6 dan yuqori silkinish zilzila hisoblanadi
  return (delta > 0.6f);
}

// --- SERVERGA YUBORISH ---
void sendData(bool alarm, bool fire, bool quake, float v6, float v9, float v7, float temp) {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClientSecure client;
    client.setInsecure(); // Hozircha sertifikatni tekshirmaymiz (oson ulanish uchun)

    HTTPClient http;
    // HTTPS va HTTP farqini avtomatik aniqlash
    if (String(SERVER_URL).startsWith("https")) {
      http.begin(client, SERVER_URL);
    } else {
      http.begin(SERVER_URL);
    }

    http.addHeader("Content-Type", "application/json");
    http.setTimeout(5000); // Timeoutni oshiramiz (HTTPS sekinroq bo'lishi mumkin)

    char json[200];
    snprintf(json, sizeof(json),
      "{\"alarm\":%s,\"fire\":%s,\"quake\":%s,\"lpg\":%.2f,\"gas\":%.2f,\"co\":%.2f,\"temp\":%.1f}",
      alarm ? "true":"false", fire ? "true":"false", quake ? "true":"false",
      v6, v9, v7, temp
    );

    int code = http.POST(json);
    if(code > 0) Serial.printf("Server Javobi: %d\n", code);
    else Serial.printf("Server Xatosi: %s\n", http.errorToString(code).c_str());

    http.end();
  }
}

// --- SETUP ---
void setup() {
  Serial.begin(115200);
  delay(1000); // Serial port ochilishi uchun kutish
  Serial.println("\n\n=========================================");
  Serial.println("=== GOLD STANDARD SYSTEM INITIALIZING ===");
  Serial.println("=========================================");

  // Watchdog Timer yoqilishi (Safety)
  esp_task_wdt_init(WDT_TIMEOUT, true);
  esp_task_wdt_add(nullptr);

  pinMode(PIN_MQ6_DO, INPUT);
  pinMode(PIN_MQ9_DO, INPUT);
  pinMode(PIN_MQ7_DO, INPUT);
  pinMode(PIN_FIRE_DO, INPUT); // Digital o'qish uchun

  // I2C boshlash
  Wire.begin(I2C_LCD_SDA, I2C_LCD_SCL); // LCD
  Wire.setTimeOut(2000); // LCD I2C Timeout
  lcd.init();
  lcd.backlight();
  lcd.print("SYSTEM STARTING");
  Serial.println("[INIT] LCD Yuklandi");

  initMPU(); // MPU
  Serial.println("[INIT] MPU6050 Yuklandi");

  // Kalibratsiya
  Serial.println("[INIT] Sensorlar kalibratsiyasi boshlandi...");
  lcd.setCursor(0,1); lcd.print("Kalibratsiya...");
  for(int i=0; i<30; i++) { // 3 soniya (30 * 100ms)
    baseMq6 += readAdcVoltage(PIN_MQ6_AO);
    baseMq9 += readAdcVoltage(PIN_MQ9_AO);
    baseMq7 += readAdcVoltage(PIN_MQ7_AO);
    checkQuake(); // Dastlabki silkinishlarni ignor qilish
    delay(100);
    esp_task_wdt_reset(); // Watchdogga "tirikmiz" deyish
  }
  baseMq6 /= 30.0;
  baseMq9 /= 30.0;
  baseMq7 /= 30.0;
  Serial.printf("[INFO] Base V: MQ6=%.2f, MQ9=%.2f, MQ7=%.2f\n", baseMq6, baseMq9, baseMq7);

  // WiFi
  lcd.clear();
  lcd.print("WiFi Ulanmoqda..");
  Serial.println("[INIT] WiFi Ulanmoqda...");
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  // WiFi ni qotib qolmasdan kutish (max 3 sek)
  long startW = millis();
  while(WiFi.status() != WL_CONNECTED && millis() - startW < 5000) { // 5 sek kutish
    delay(100);
    esp_task_wdt_reset();
    Serial.print(".");
  }
  Serial.println();
  if(WiFi.status() == WL_CONNECTED) {
      Serial.println("[SUCCESS] WiFi Ulandi! IP: " + WiFi.localIP().toString());
  } else {
      Serial.println("[WARNING] WiFi Ulanmadi (Offline rejim)");
  }
  lcd.clear();
}

// --- LOOP ---
void loop() {
  esp_task_wdt_reset(); // Tizim tirik, reset berma!

  // 1. Tarmoqni tekshirish (uzilsa qayta ulaydi)
  if (WiFi.status() != WL_CONNECTED && millis() % 10000 < 50) {
     WiFi.disconnect();
     WiFi.reconnect();
  }

  // 2. O'qish
  float v6 = readAdcVoltage(PIN_MQ6_AO);
  float v9 = readAdcVoltage(PIN_MQ9_AO);
  float v7 = readAdcVoltage(PIN_MQ7_AO);
  float temp = getTemp();
  bool fire = digitalRead(PIN_FIRE_DO) == LOW;

  // 3. Xavf logikasi
  // Digital sensordan 'LOW' kelsa yoki kuchlanish oshsa
  bool gasDanger = (digitalRead(PIN_MQ6_DO)==LOW || v6 > baseMq6 + SENS_THRESH ||
                    digitalRead(PIN_MQ9_DO)==LOW || v9 > baseMq9 + SENS_THRESH ||
                    digitalRead(PIN_MQ7_DO)==LOW || v7 > baseMq7 + SENS_THRESH);

  bool quake = checkQuake();
  bool alarm = fire || gasDanger || quake;

  // 4. Ekran (Har 250ms yangilanadi yoki Xavf paytida darhol)
  if (millis() - lastLcdTime > 250) {
    lastLcdTime = millis();
    lcd.setCursor(0,0);
    if (alarm) {
      if(fire)      lcd.print("!! YONG'IN !!   ");
      else if(quake)lcd.print("!! ZILZILA !!   ");
      else          lcd.print("!! GAZ XAVFI !! ");

      lcd.setCursor(0,1); lcd.print("XAVF ANIQLANDI! ");
    } else {
      // Normal holat
      lcd.printf("L:%.1f G:%.1f C:%.1f", v6, v9, v7);
      lcd.setCursor(0,1);
      lcd.printf("T:%.0fC W:%s", temp, WiFi.status()==WL_CONNECTED?"Ok":"..");
    }
  }

  // 5. Serverga yuborish
  // Xavf paytida har 3 soniyada, tinch paytda har 60 soniyada
  unsigned long interval = alarm ? 3000 : 60000;
  if (millis() - lastSentTime > interval) {
    sendData(alarm, fire, quake, v6, v9, v7, temp);
    lastSentTime = millis();

    // Debug
    Serial.printf("ST: G(%.2f) F(%d) Q(%d) W(%d)\n", v6, fire, quake, WiFi.status());
  }

  delay(10); // CPU yuklamasini kamaytirish
}
