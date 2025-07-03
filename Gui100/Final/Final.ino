#include <WiFi.h>
#include <AsyncMQTT_ESP32.h>
#include <Ticker.h>
#include <DHT.h>  // THU VIEN DHT

// THONG TIN WIFI
const char* ssid = "3508B 2.4G";
const char* password = "12345668";

// MQTT BROKER
#define MQTT_HOST IPAddress(10, 136, 201, 229)
#define MQTT_PORT 1883

// CAU HINH DHT11
#define DHTPIN 2      
#define DHTTYPE DHT11 
DHT dht(DHTPIN, DHTTYPE);

AsyncMqttClient mqttClient;
Ticker mqttReconnectTimer;
Ticker wifiReconnectTimer;

#define MAX_MESSAGES 100

unsigned long sentTimestamps[MAX_MESSAGES];
unsigned long durations[MAX_MESSAGES];
uint16_t packetIds[MAX_MESSAGES];
int successCount = 0;
int totalSent = 0;
int timeoutCount = 0;  // TIMEOUT

// HAM XU LY KHI NHAN DUOC PUBACK
void onMqttPublish(uint16_t packetId) {
  Serial.print("NHAN PUBACK - Packet ID: ");
  Serial.print(packetId);

  for (int i = 0; i < totalSent; i++) {
    if (packetIds[i] == packetId && durations[i] == 0) {
      durations[i] = millis() - sentTimestamps[i];
      successCount++;
      
      Serial.print(" THOI GIAN: ");
      Serial.print(durations[i]);
      Serial.print("ms (");
      Serial.print(successCount);
      Serial.print("/");
      Serial.print(totalSent);
      Serial.println(")");
      break;
    }
  }

  // THONG KE KHI NHAN DU 100 GOI TIN (SUCCESS + TIMEOUT)
  if ((successCount + timeoutCount) == MAX_MESSAGES) {
    showStatistics();
  }
}

void showStatistics() {
  // LOẠI BỎ TIMEOUT
  unsigned long minTime = 999999;
  unsigned long maxTime = 0;
  unsigned long sum = 0;
  int validCount = 0;

  for (int i = 0; i < MAX_MESSAGES; i++) {
    if (durations[i] > 0 && durations[i] < 99999) {  // Bỏ qua timeout
      if (durations[i] < minTime) minTime = durations[i];
      if (durations[i] > maxTime) maxTime = durations[i];
      sum += durations[i];
      validCount++;
    }
  }

  Serial.println("\n===== THONG KE HIEU NANG MQTT =====");
  Serial.print("TONG GOI TIN GUI: "); Serial.println(totalSent);
  Serial.print("NHAN PUBACK: "); Serial.println(successCount);
  Serial.print("TIMEOUT: "); Serial.println(timeoutCount);
  Serial.print("TY LE THANH CONG: "); Serial.print((float)successCount/totalSent*100, 1); Serial.println("%");
  
  if (validCount > 0) {
    Serial.print("THOI GIAN PUBACK (min/max/avg): ");
    Serial.print(minTime); Serial.print("ms / ");
    Serial.print(maxTime); Serial.print("ms / ");
    Serial.print((float)sum/validCount, 2); Serial.println("ms");
  }
  Serial.println("=====================================\n");
}

// Ket noi WiFi
void connectToWifi() {
  Serial.println("DANG KET NOI WIFI...");
  WiFi.begin(ssid, password);
}

// Ket noi MQTT
void connectToMqtt() {
  Serial.println("DANG KET NOI MQTT...");
  mqttClient.connect();
}

void onMqttConnect(bool sessionPresent) {
  Serial.println("MQTT DA KET NOI THANH CONG!");
  Serial.println("BAT DAU GUI DU LIEU...\n");
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  Serial.println("MQTT BI NGAT KET NOI");
  if (WiFi.isConnected()) {
    mqttReconnectTimer.once(2, connectToMqtt);
  }
}

void onWiFiConnect(WiFiEvent_t event) {
  Serial.println("WIFI DA KET NOI!");
  Serial.print("   IP address: ");
  Serial.println(WiFi.localIP());
  connectToMqtt();
}

void onWiFiDisconnect(WiFiEvent_t event) {
  Serial.println("WIFI BI NGAT, DANG THU KET NOI LAI...");
  mqttReconnectTimer.detach();
  wifiReconnectTimer.once(2, connectToWifi);
}

void setup() {
  Serial.begin(115200);
  Serial.println("\nESP32 MQTT Performance Test");
  Serial.println("================================");
  Serial.println("DO THOI GIAN PUBACK CHINH XAC");
  Serial.println("MUC TIEU: 100 GOI TIN MQTT");
  Serial.println("================================\n");

  // Khởi tạo cảm biến DHT11
  dht.begin();
  Serial.println("Cảm biến DHT11 đã được khởi tạo");

  WiFi.onEvent(onWiFiConnect, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_GOT_IP);
  WiFi.onEvent(onWiFiDisconnect, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
  
  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.onPublish(onMqttPublish);
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  mqttClient.setKeepAlive(60);
  mqttClient.setCleanSession(true);

  connectToWifi();
}

void loop() {
  static unsigned long lastSent = 0;
  static unsigned long lastCheck = 0;

  // KIEM TRA TIMEOUT PUBACK MOI 10 GIAY
  if (millis() - lastCheck > 10000) {
    for (int i = 0; i < totalSent; i++) {
      if (durations[i] == 0 && (millis() - sentTimestamps[i]) > 15000) {
        Serial.print("TIMEOUT Packet ID: ");
        Serial.print(packetIds[i]);
        Serial.println(" (khong nhan duoc PUBACK)");
        durations[i] = 99999;  // Danh dau timeout
        timeoutCount++;  //  TĂNG ĐẾM TIMEOUT
      }
    }
    lastCheck = millis();
  }

  // GUI MESSAGE MOI 5 GIAY
  if (millis() - lastSent > 5000 && mqttClient.connected() && totalSent < MAX_MESSAGES) {
    int pendingCount = totalSent - successCount - timeoutCount;
    if (pendingCount > 5) {
      Serial.print("Qua nhieu PUBACK pending (");
      Serial.print(pendingCount);
      Serial.println("), tam dung gui...");
      return;
    }
    
    // Đọc nhiệt độ từ cảm biến DHT11
    float temp = dht.readTemperature();
    float humidity = dht.readHumidity();
    
    // Kiểm tra xem việc đọc có thành công không
    if (isnan(temp) || isnan(humidity)) {
      Serial.println("Không thể đọc dữ liệu từ cảm biến DHT11!");
      temp = -999; // Giá trị cho biết lỗi
    } else {
      Serial.print("Nhiệt độ: ");
      Serial.print(temp);
      Serial.print("°C, Độ ẩm: ");
      Serial.print(humidity);
      Serial.println("%");
    }
    
    char payload[100];
    sprintf(payload, "{\"temp\":%.1f,\"humidity\":%.1f,\"msg\":%u,\"time\":%lu}", 
            temp,
            humidity,
            totalSent + 1, 
            millis());
            
    sentTimestamps[totalSent] = millis();
    durations[totalSent] = 0;
    
    uint16_t packetId = mqttClient.publish("esp32/sensor_data", 1, false, payload);
    packetIds[totalSent] = packetId;
    totalSent++;

    Serial.print("Goi Tin[");
    Serial.print(totalSent);
    Serial.print("/");
    Serial.print(MAX_MESSAGES);
    Serial.print("] Packet ID: ");
    Serial.print(packetId);
    Serial.print(" | Pending: ");
    Serial.println(pendingCount + 1);

    lastSent = millis();
  }
  
  // DUNG CHUONG TRINH KHI HOAN THANH
  if ((successCount + timeoutCount) == MAX_MESSAGES && totalSent == MAX_MESSAGES) {
    Serial.println("\nHOAN THANH TEST - CHUONG TRINH DUNG!");
    Serial.println("Nhan reset de chay lai...\n");
    while(true) {
      delay(1000);
    }
  }
}