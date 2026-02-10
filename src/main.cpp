#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>

// --- ULTIMATE GOLD IOT SYSTEM ---
// "Peak Performance" Architecture
// --------------------------------

// 1. DATCHIK PINLARI
#define PIN_MQ6_AO   34  // LPG Analog
#define PIN_MQ6_DO   25  // LPG Digital (Alarm)
#define PIN_MQ9_AO   33  // Gaz Analog
#define PIN_MQ9_DO   14  // Gaz Digital (Alarm)
#define PIN_MQ7_AO   32  // CO Analog
#define PIN_MQ7_DO   27  // CO Digital (Alarm)
#define PIN_FIRE_DO  35  // Yong'in

// 2. SERVER VA WIFI SOZLAMALARI (O'zingiznikiga o'zgartiring)
const char* WIFI_SSID = "WiFi_Nomi";      // <-- O'ZGARTIRING
const char* WIFI_PASS = "WiFi_Paroli";    // <-- O'ZGARTIRING
const char* SERVER_URL = "http://example.com/api/data"; // <-- O'ZGARTIRING

// 3. HARDWARE CONSTANTS
#define GAZ_ALARM_VOLT 2.5
// Chegara: Agar toza havodan shahancha V ga oshsa signal beradi
#define SENSITIVITY 0.8

LiquidCrystal_I2C lcd(0x27, 16, 4);
TwoWire I2C_MPU = TwoWire(1);

// 4. GLOBAL STATE
unsigned long lastNetworkCheck = 0;
unsigned long lastDataSent = 0;
unsigned long timerLCD = 0;
bool mpuInitialized = false;
float lastX, lastY, lastZ;

// Kalibratsiya uchun o'zgaruvchilar
float baseMQ6 = 0.0;
float baseMQ9 = 0.0;
float baseMQ7 = 0.0;

// --- YORDAMCHI: WIFI ULANISH (NON-BLOCKING) ---
void checkNetwork() {
  // Har 10 sekundda tarmoq holatini tekshiramiz
  if (millis() - lastNetworkCheck > 10000) {
    lastNetworkCheck = millis();
    if (WiFi.status() != WL_CONNECTED) {
      WiFi.disconnect();
      WiFi.begin(WIFI_SSID, WIFI_PASS);
      // Eslatma: Bu yerda while() bilan kutmaymiz, tizim qotib qolmasligi kerak
    }
  }
}

// --- YORDAMCHI: MA'LUMOT YUBORISH (HTTP POST) ---
void sendDataToServer(bool isAlarm, bool fire, bool gas, bool quake, float vMQ6, float vMQ7, float temp) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(SERVER_URL);
    http.addHeader("Content-Type", "application/json");

    // JSON QURISH (Tezkor va yengil usul)
    // Format: {"id":"ESP_HOME","alarm":true,"fire":false,"gas":true,"lpg":1.2,"co":0.5, "temp": 25.5}
    char jsonBuffer[150];
    snprintf(jsonBuffer, sizeof(jsonBuffer),
             "{\"id\":\"ESP_HOME\",\"alarm\":%s,\"fire\":%s,\"gas\":%s,\"quake\":%s,\"lpg\":%.2f,\"co\":%.2f,\"temp\":%.1f}",
             isAlarm ? "true" : "false",
             fire ? "true" : "false",
             gas ? "true" : "false",
             quake ? "true" : "false",
             vMQ6, vMQ7, temp);

    // Yuborish (Async emas, lekin qisqa vaqt oladi)
    int httpResponseCode = http.POST(jsonBuffer);
    http.end();
  }
}

// --- SIGNALNI FILTRLASH ---
float readStableVoltage(int pin) {
  long sum = 0;
  for(int i=0; i<10; i++) { // Aniqroq bo'lishi uchun 5 dan 10 ga oshirdik
    sum += analogRead(pin);
    delay(2);
  }
  return (sum / 10.0) * (3.3 / 4095.0);
}

// --- KALIBRATSIYA FUNKSIYASI ---
void calibrateSensors() {
  lcd.clear();
  lcd.print("Kalibratsiya...");
  lcd.setCursor(0,1); lcd.print("Kuting: 10s");

  float s6 = 0, s9 = 0, s7 = 0;
  int count = 50;

  for(int i=0; i<count; i++) {
    s6 += analogRead(PIN_MQ6_AO);
    s9 += analogRead(PIN_MQ9_AO);
    s7 += analogRead(PIN_MQ7_AO);
    delay(200); // 50 * 200ms = 10 soniya

    // Progress bar
    if(i % 5 == 0) {
       lcd.setCursor(i/5, 2);
       lcd.print(".");
    }
  }

  // O'rtacha kuchlanishni (Bazaviy nol nuqta) hisoblash
  baseMQ6 = (s6 / count) * (3.3 / 4095.0);
  baseMQ9 = (s9 / count) * (3.3 / 4095.0);
  baseMQ7 = (s7 / count) * (3.3 / 4095.0);

  lcd.clear();
  lcd.print("Kalibrlandi!");
  delay(1000);
}

// --- MPU SETUP ---
void initMPU() {
  I2C_MPU.begin(19, 18, 400000);
  I2C_MPU.beginTransmission(0x68);
  I2C_MPU.write(0x6B); I2C_MPU.write(0);
  if (I2C_MPU.endTransmission(true) == 0) mpuInitialized = true;
}

// --- MPU HARORATINI O'QISH ---
float getMPUTemperature() {
  if (!mpuInitialized) return 0.0;
  I2C_MPU.beginTransmission(0x68);
  I2C_MPU.write(0x41); // TEMP_OUT_H
  I2C_MPU.endTransmission(false);
  I2C_MPU.requestFrom(0x68, 2, true);

  if (I2C_MPU.available() >= 2) {
    int16_t rawh = I2C_MPU.read() << 8 | I2C_MPU.read();
    return (rawh / 340.0) + 36.53; // MPU6050 formulasi
  }
  return 0.0;
}

// --- ZILZILA LOGIKASI ---
bool checkEarthquake() {
  if (!mpuInitialized) return false;
  I2C_MPU.beginTransmission(0x68);
  I2C_MPU.write(0x3B);
  I2C_MPU.endTransmission(false);
  I2C_MPU.requestFrom(0x68, 6, true);
  if (I2C_MPU.available() < 6) return false;

  int16_t x = I2C_MPU.read()<<8|I2C_MPU.read();
  int16_t y = I2C_MPU.read()<<8|I2C_MPU.read();
  int16_t z = I2C_MPU.read()<<8|I2C_MPU.read();

  float ax=x/16384.0; float ay=y/16384.0; float az=z/16384.0;
  float delta = abs(ax-lastX) + abs(ay-lastY) + abs(az-lastZ);
  lastX=ax; lastY=ay; lastZ=az;

  return (delta > 0.5 && delta < 10.0);
}

void setup() {
  Serial.begin(115200);

  // Pinlar
  pinMode(PIN_FIRE_DO, INPUT);
  pinMode(PIN_MQ6_DO, INPUT);
  pinMode(PIN_MQ9_DO, INPUT);
  pinMode(PIN_MQ7_DO, INPUT);

  // LCD
  Wire.begin(21, 22);
  lcd.begin();
  lcd.backlight();
  lcd.print("SYSTEM STARTING");

  // MPU
  initMPU();
  checkEarthquake();

  // --- KALIBRATSIYA QILISH ---
  // Tizim yonishi bilan hozirgi havoni "toza" deb oladi
  calibrateSensors();

  // WiFi Boshlash
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  lcd.setCursor(0,1); lcd.print("Wi-Fi Connecting");

  // Bir muddat kutamiz, lekin majburiy emas
  unsigned long startWait = millis();
  while(WiFi.status() != WL_CONNECTED && millis() - startWait < 3000) {
    delay(100);
  }
  lcd.clear();
}

void loop() {
  // 1. TARMOQNI FONDA BOSHQARISH
  checkNetwork();

  // 2. DATCHIKLARNI O'QISH
  bool fire = (digitalRead(PIN_FIRE_DO) == LOW);
  float vMQ6 = readStableVoltage(PIN_MQ6_AO);
  float vMQ9 = readStableVoltage(PIN_MQ9_AO);
  float vMQ7 = readStableVoltage(PIN_MQ7_AO);

  // ALARM MANTIQI:
  // 1. Digital Pin (Datchikni o'zini sozlagichi) signal bersa
  // 2. YOKI: Hozirgi kuchlanish > (Bazaviy kuchlanish + Sezgirlik) bo'lsa
  bool alarmGas = (digitalRead(PIN_MQ6_DO)==LOW || vMQ6 > (baseMQ6 + SENSITIVITY) ||
                   digitalRead(PIN_MQ9_DO)==LOW || vMQ9 > (baseMQ9 + SENSITIVITY) ||
                   digitalRead(PIN_MQ7_DO)==LOW || vMQ7 > (baseMQ7 + SENSITIVITY));

  bool earthquake = checkEarthquake();

  // 3. XAVFSIZLIK LOGIKASI (ALARM)
  if (fire || alarmGas || earthquake) {
     // Ekranni darhol yangilash
     if (millis() - timerLCD > 200) {
        lcd.clear();
        lcd.setCursor(0,0);
        if(fire) lcd.print("!!! YONG'IN !!!");
        else if(earthquake) lcd.print("!!! ZILZILA !!!");
        else lcd.print("!!! GAZ XAVFI !!!");

        lcd.setCursor(0,1); lcd.print("SERVERGA YUBORILDI");
        timerLCD = millis();
     }

     // Serverga signal yuborish (Xavf paytida har 3 sekundda)
     if (millis() - lastDataSent > 3000) {
       sendDataToServer(true, fire, alarmGas, earthquake, vMQ6, vMQ7, getMPUTemperature());
       lastDataSent = millis();
     }
     return;
  }

  // 4. TINCH HOLAT (MONITORING)
  if (millis() - timerLCD > 500) {
    timerLCD = millis();
    lcd.setCursor(0,0); lcd.printf("LPG:%.1fV CO:%.1fV", vMQ6, vMQ7);

    // Haroratni va WiFi ni chiqarish
    float temp = getMPUTemperature();

    lcd.setCursor(0,1);
    lcd.printf("Temp:%.1fC ", temp);
    if(WiFi.status() == WL_CONNECTED) lcd.print("Wi:ON ");
    else lcd.print("Wi:OFF");

    // --- SERIAL MONITORGA CHIQARISH (DEBUG) ---
    Serial.println("--- SENSORLAR HOLATI ---");
    // Kalibratsiyaga nisbatan farqni ko'rsatamiz
    Serial.printf("LPG (MQ6): %.2f V (Base: %.2f)\n", vMQ6, baseMQ6);
    Serial.printf("Gaz (MQ9): %.2f V (Base: %.2f)\n", vMQ9, baseMQ9);
    Serial.printf("CO  (MQ7): %.2f V (Base: %.2f)\n", vMQ7, baseMQ7);
    Serial.printf("Temp: %.1f C\n", temp);
    Serial.printf("Yong'in: %s\n", fire ? "HA" : "YO'Q");
    Serial.printf("Zilzila: %s\n", earthquake ? "HA" : "YO'Q");
    Serial.printf("WiFi: %s\n", WiFi.status() == WL_CONNECTED ? "ULANGAN" : "UZILGAN");
    Serial.println("--------------------------");

    // Serverga har 30 sekundda hisobot
    if (millis() - lastDataSent > 30000) {
       sendDataToServer(false, false, false, false, vMQ6, vMQ7, temp);
       lastDataSent = millis();
    }
  }
}
