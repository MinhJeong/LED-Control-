#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include "LEDPatternController.h"

// WiFi Configuration
const char* ssid = "3508B 2.4G";
const char* password = "12345668";
const char* hostname = "esp32-led";

// LED Configuration
const int LED_PIN = 2;  // LED built-in ESP32
const unsigned long PATTERN_INTERVAL = 500; // 0.5 seconds per bit

// Web server và controller
WebServer server(80);
LEDPatternController ledController(&server, LED_PIN, PATTERN_INTERVAL);

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n=== ESP32 LED Pattern Controller ===");
  
  // Cấu hình LED
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH); // Tắt LED ban đầu
    // Connect to WiFi with multiple strategies (max 30 attempts total)
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);
    int totalAttempts = 0;
  const int MAX_TOTAL_ATTEMPTS = 60;
  bool wifiConnected = false;
  
  // Strategy 1: Normal connection (25 attempts)
  Serial.println("📡 Strategy 1: Normal connection...");
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(hostname);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED && totalAttempts < 25) {
    delay(500);
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
    Serial.print(".");
    totalAttempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
  } else {
    // Strategy 2: Reset and retry (20 more attempts)
    Serial.println("\n📡 Strategy 2: Reset and retry...");
    WiFi.disconnect();
    delay(1000);
    WiFi.begin(ssid, password);
    
    while (WiFi.status() != WL_CONNECTED && totalAttempts < 45) {
      delay(500);
      digitalWrite(LED_PIN, !digitalRead(LED_PIN));
      Serial.print(".");
      totalAttempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
      wifiConnected = true;
    } else {
      // Strategy 3: Force reconnect (15 final attempts)
      Serial.println("\n📡 Strategy 3: Force reconnect...");
      WiFi.mode(WIFI_OFF);
      delay(1000);
      WiFi.mode(WIFI_STA);
      WiFi.setHostname(hostname);
      WiFi.begin(ssid, password);
      
      while (WiFi.status() != WL_CONNECTED && totalAttempts < MAX_TOTAL_ATTEMPTS) {
        delay(500);
        digitalWrite(LED_PIN, !digitalRead(LED_PIN));
        Serial.print(".");
        totalAttempts++;
      }
      
      if (WiFi.status() == WL_CONNECTED) {
        wifiConnected = true;
      }
    }
  }
  
  if (wifiConnected) {
    digitalWrite(LED_PIN, HIGH); // Turn off LED
    Serial.println("\n✅ WiFi connected successfully!");
    Serial.print("📶 IP address: ");
    Serial.println(WiFi.localIP());
    Serial.print("📡 Signal strength: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
    Serial.print("🔢 Total attempts: ");
    Serial.println(totalAttempts);
    
    // Configure mDNS
    if (MDNS.begin(hostname)) {
      Serial.print("🌐 mDNS started: http://");
      Serial.print(hostname);
      Serial.println(".local");
    }
    
    // Initialize LED controller
    ledController.setup();
    
    // Start web server
    server.begin();
    
    Serial.println("🚀 Web server started!");
    Serial.println("📋 Access points:");
    Serial.println("   • http://" + WiFi.localIP().toString());
    Serial.println("   • http://" + String(hostname) + ".local");
    Serial.println("=====================================");
    
    // Success LED effect (3 fast blinks)
    for (int i = 0; i < 6; i++) {
      digitalWrite(LED_PIN, !digitalRead(LED_PIN));
      delay(200);
    }
    digitalWrite(LED_PIN, HIGH); // Turn off LED
    
  } else {    // All strategies failed - switch to Access Point mode
    Serial.println("\n❌ WiFi connection failed after 60 attempts!");
    Serial.println("🔄 Switching to Access Point mode...");
    
    digitalWrite(LED_PIN, HIGH); // Turn off LED
    
    // Configure Access Point
    String apSSID = "ESP32-LED-" + String((uint32_t)ESP.getEfuseMac(), HEX);
    String apPassword = "12345678";
    
    WiFi.mode(WIFI_AP);
    WiFi.softAP(apSSID.c_str(), apPassword.c_str());
    
    Serial.println("📶 Access Point created:");
    Serial.println("   SSID: " + apSSID);
    Serial.println("   Password: " + apPassword);
    Serial.print("   IP: ");
    Serial.println(WiFi.softAPIP());
    
    // Initialize LED controller
    ledController.setup();
    
    // Start web server
    server.begin();
    
    Serial.println("🚀 Web server started in AP mode!");
    Serial.println("📋 Connect to WiFi '" + apSSID + "' and visit:");
    Serial.println("   • http://" + WiFi.softAPIP().toString());
    Serial.println("=====================================");
    
    // AP mode LED effect (slow continuous blink)
    for (int i = 0; i < 10; i++) {
      digitalWrite(LED_PIN, !digitalRead(LED_PIN));
      delay(500);
    }
    digitalWrite(LED_PIN, HIGH); // Turn off LED
  }
}

void loop() {
  // Ultra-high-speed main loop for maximum LED timing precision
  
  // Handle LED pattern execution (highest priority - called multiple times for high frequencies)
  ledController.handleLoop();
  
  // Handle web server requests frequently but not on every loop iteration
  static unsigned long lastWebHandle = 0;
  unsigned long currentMicros = micros();
  
  // Handle web requests every 1ms (1000 microseconds) for good responsiveness
  if (currentMicros - lastWebHandle >= 1000) {
    server.handleClient();
    lastWebHandle = currentMicros;
  }
  
  // Check WiFi connection only if in STA mode (much less frequent)
  if (WiFi.getMode() == WIFI_STA) {
    static unsigned long lastWiFiCheck = 0;
    unsigned long currentMillis = millis();
    
    if (currentMillis - lastWiFiCheck > 30000) { // Check every 30 seconds
      lastWiFiCheck = currentMillis;
      
      if (WiFi.status() != WL_CONNECTED) {
        Serial.println("🔄 WiFi disconnected! Trying to reconnect...");
        WiFi.reconnect();
        
        // Non-blocking reconnection check
        static unsigned long reconnectStartTime = 0;
        static bool reconnectInProgress = false;
        
        if (!reconnectInProgress) {
          reconnectStartTime = currentMillis;
          reconnectInProgress = true;
        }
        
        // Check reconnection status after 5 seconds without blocking
        if (currentMillis - reconnectStartTime > 5000) {
          reconnectInProgress = false;
          
          if (WiFi.status() != WL_CONNECTED) {
            Serial.println("❌ Reconnection failed! Switching to AP mode...");
            
            // Switch to Access Point mode
            String apSSID = "ESP32-LED-" + String((uint32_t)ESP.getEfuseMac(), HEX);
            String apPassword = "12345678";
            
            WiFi.mode(WIFI_AP);
            WiFi.softAP(apSSID.c_str(), apPassword.c_str());
            
            Serial.println("📶 Switched to Access Point mode:");
            Serial.println("   SSID: " + apSSID);
            Serial.println("   Password: " + apPassword);
            Serial.print("   IP: ");
            Serial.println(WiFi.softAPIP());
          } else {
            Serial.println("✅ WiFi reconnected successfully!");
          }
        }
      }
    }
  }
  
  // Feed watchdog without delay - use yield() for cooperative multitasking
  yield();
}
