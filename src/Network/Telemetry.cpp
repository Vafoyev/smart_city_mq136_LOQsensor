#include "Network/Telemetry.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

Telemetry::Telemetry(const char* ssid, const char* password, const char* serverUrl, const char* token)
    : _ssid(ssid), _password(password), _serverUrl(serverUrl), _token(token), _lastConnectionCheck(0) {}

void Telemetry::begin() {
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(_ssid, _password);
}

void Telemetry::handleConnection() {
  // Check and reconnect every 10 seconds if not connected
  if (WiFi.status() != WL_CONNECTED && (millis() - _lastConnectionCheck > 10000)) {
    _lastConnectionCheck = millis();
    Serial.println("[WIFI] Reconnecting to WiFi...");
    WiFi.disconnect();
    WiFi.reconnect();
  }
}

bool Telemetry::isConnected() const {
  return (WiFi.status() == WL_CONNECTED);
}

bool Telemetry::sendPayload(bool alarm, bool fire, bool quake, float mq6, float mq7, float temp) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[TELEMETRY] Cannot send payload, WiFi not connected");
    return false;
  }

  WiFiClientSecure client;
  client.setInsecure(); // Skip certificate verification for simple setup

  HTTPClient http;
  if (String(_serverUrl).startsWith("https")) {
    http.begin(client, _serverUrl);
  } else {
    http.begin(_serverUrl);
  }

  http.addHeader("Content-Type", "application/json");
  http.setTimeout(5000); // 5s timeout (HTTPS can be slow)

  char json[256];
  snprintf(json, sizeof(json),
    "{\"alarm\":%s,\"fire\":%s,\"quake\":%s,\"lpg\":%.2f,\"co\":%.2f,\"temp\":%.1f,\"token\":\"%s\"}",
    alarm ? "true" : "false", 
    fire ? "true" : "false", 
    quake ? "true" : "false",
    mq6, mq7, temp, _token
  );

  int httpCode = http.POST(json);
  bool success = false;
  
  if (httpCode > 0) {
    Serial.printf("[TELEMETRY] Server response code: %d\n", httpCode);
    success = (httpCode >= 200 && httpCode < 300);
  } else {
    Serial.printf("[TELEMETRY] Error sending POST: %s\n", http.errorToString(httpCode).c_str());
  }

  http.end();
  return success;
}
