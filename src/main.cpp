#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <math.h>

// --- Sozlamalar ---
#define MQ6_ANALOG_PIN  34
#define FIRE_PIN        14
#define I2C_SDA_PIN 23
#define I2C_SCL_PIN 21
int lcdAddress = 0x27;
int lcdColumns = 16;
int lcdRows = 4;
#define VCC 5.0f
#define RL_VALUE 10000.0f
const float R0_VALUE = 285000.0f;
const float GAS_LPG_A = 265.82;
const float GAS_LPG_B = -1.729;

LiquidCrystal_I2C lcd(lcdAddress, lcdColumns, lcdRows);

float getPPM(int analog_value) {
  if (analog_value < 1) return 0;
  float sensor_voltage = analog_value * (3.3f / 4095.0f);
  if (sensor_voltage == 0) return 0;
  float sensor_resistance = ((VCC * RL_VALUE) / sensor_voltage) - RL_VALUE;
  if (sensor_resistance < 0) sensor_resistance = 0;
  float ratio = sensor_resistance / R0_VALUE;
  float ppm = GAS_LPG_A * pow(ratio, GAS_LPG_B);
  if (ppm < 0) ppm = 0;
  return ppm;
}

void setup() {
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  pinMode(FIRE_PIN, INPUT);

  lcd.begin();
  lcd.backlight();
  lcd.leftToRight();
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("Gaz Miqdori:");

  lcd.setCursor(0, 2);
  lcd.print("Yongin Holati:");
}

void loop() {
  int gas_analog_val = analogRead(MQ6_ANALOG_PIN);
  bool isFireDetected = digitalRead(FIRE_PIN);
  float gas_ppm = getPPM(gas_analog_val);

  char text_buffer[17];

  sprintf(text_buffer, "%d PPM", (int)gas_ppm);
  lcd.setCursor(0, 1);
  lcd.print("                ");
  lcd.setCursor(0, 1);
  lcd.print(text_buffer);

  if (isFireDetected == HIGH) {
    sprintf(text_buffer, "DIQQAT, YONG'IN!");
  } else {
    sprintf(text_buffer, "XAVFSIZ");
  }
  lcd.setCursor(0, 3);
  lcd.print("                ");
  lcd.setCursor(0, 3);
  lcd.print(text_buffer);

  delay(1000);
}