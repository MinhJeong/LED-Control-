#include "LEDPatternController.h"
#include <driver/gpio.h>

LEDPatternController::LEDPatternController(WebServer* server, int ledPin, unsigned long interval) {
  this->server = server;
  this->ledPin = ledPin;
  this->patternInterval = interval;
  this->currentPattern = "1010"; // Default test pattern
  this->currentIndex = 0;
  this->customFrequencyHz = 2; // Default 2Hz
  this->useCustomFrequency = false; // Start in default mode
  this->softwareTimerActive = true; // Always use software timer
  
  //  IMMEDIATE TIMING INITIALIZATION
  this->lastMicros = micros();
  this->lastPatternTime = millis();
  
  //  OPTIMIZE GPIO FOR HIGH SPEED
  pinMode(this->ledPin, OUTPUT);
  gpio_set_drive_capability((gpio_num_t)this->ledPin, GPIO_DRIVE_CAP_3); // Max drive strength
  digitalWrite(this->ledPin, HIGH); // Start with LED off
  
  Serial.println(" LEDPatternController initialized - ULTRA-HIGH-SPEED software timer");
  Serial.println(" Ready for instant pattern execution with microsecond precision");
}

void LEDPatternController::setup() {
  registerEndpoints();
}

int LEDPatternController::countBits(char bit) {
  int count = 0;
  for (int i = 0; i < currentPattern.length(); i++) {
    if (currentPattern.charAt(i) == bit) {
      count++;
    }
  }
  return count;
}

void LEDPatternController::setCustomFrequency(unsigned long frequencyHz) {
  Serial.println(" Setting high-speed software timer: " + String(frequencyHz) + " Hz");
  
  //  IMMEDIATE FREQUENCY CHANGE - No delays
  if (frequencyHz > 0) {
    this->useCustomFrequency = true;
    this->customFrequencyHz = frequencyHz;
    Serial.println(" High-speed mode: " + String(frequencyHz) + " Hz (Microsecond precision)");
  } else {
    this->useCustomFrequency = false;
    this->customFrequencyHz = 2; // Default 2Hz
    Serial.println(" Default mode: 2Hz (Microsecond precision)");
  }
  
  //  INSTANT TIMING RESET - Start immediately
  this->currentIndex = 0;
  this->lastMicros = micros(); // Use microseconds for all timing
  this->lastPatternTime = millis(); // Keep for compatibility
  
  Serial.println(" Instant timing sync - frequency active immediately");
}

void LEDPatternController::executePattern() {
  if (this->currentPattern.length() == 0) return;
  
  //  HIGH SPEED TIMING - Use microseconds for all frequencies
  unsigned long currentMicros = micros();
  unsigned long intervalMicros = 1000000UL / this->customFrequencyHz; // Always use microseconds
  
  // Check if it's time to update
  if (currentMicros - this->lastMicros >= intervalMicros) {
    char currentBit = this->currentPattern.charAt(this->currentIndex);
    
    //  INSTANT LED UPDATE - No delays
    if (currentBit == '0') {
      digitalWrite(this->ledPin, LOW);  // LED ON for VLC
    } else {
      digitalWrite(this->ledPin, HIGH); // LED OFF for VLC
    }
    
    // Move to next bit immediately
    this->currentIndex = (this->currentIndex + 1) % this->currentPattern.length();
    
    //  PRECISE TIMING SYNC - Add exact interval instead of using current time
    this->lastMicros += intervalMicros; // This prevents timing drift!
    
    // Debug output (throttled to prevent slowdown)
    static unsigned long debugCounter = 0;
    if (++debugCounter % 1000 == 0) { // Debug every 1000 executions
      Serial.println("📍 Pattern bit: " + String(currentBit) + 
                     ", Freq: " + String(this->customFrequencyHz) + "Hz" +
                     ", Interval: " + String(intervalMicros) + "μs");
    }
  }
}

void LEDPatternController::handleLoop() {
  //  ULTRA-FAST PATTERN EXECUTION
  executePattern();
  
  // Optional: Force immediate execution for high frequencies
  if (this->customFrequencyHz > 1000) {
    executePattern(); // Call twice for frequencies > 1kHz
  }
}

String LEDPatternController::generateWebPage() {
  String html = "<!DOCTYPE html>\n";
  html += "<html>\n";
  html += "<head>\n";  html += "    <title>ESP32 LED String Controller</title>\n";
  html += "    <meta charset=\"UTF-8\">\n";
  html += "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n";
  html += "    <style>\n";
  html += "        body { font-family: Arial, sans-serif; background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); margin: 0; padding: 20px; min-height: 100vh; }\n";
  html += "        .container { max-width: 600px; margin: 0 auto; background: white; padding: 30px; border-radius: 15px; box-shadow: 0 10px 30px rgba(0,0,0,0.2); }\n";
  html += "        h1 { text-align: center; color: #333; margin-bottom: 30px; }\n";
  html += "        .pattern-display { background: #f8f9fa; padding: 20px; border-radius: 10px; margin: 20px 0; text-align: center; border-left: 4px solid #667eea; }\n";
  html += "        .pattern-text { font-family: 'Courier New', monospace; font-size: 24px; font-weight: bold; color: #333; letter-spacing: 3px; }\n";
  html += "        .input-group { margin: 20px 0; }\n";
  html += "        label { display: block; margin-bottom: 10px; font-weight: bold; color: #555; }\n";
  html += "        input[type=\"text\"], input[type=\"number\"] { width: 100%; padding: 12px; border: 2px solid #ddd; border-radius: 8px; font-size: 16px; box-sizing: border-box; }\n";
  html += "        .input-row { display: grid; grid-template-columns: 2fr 1fr; gap: 15px; margin: 20px 0; }\n";
  html += "        .btn { background: #667eea; color: white; padding: 12px 30px; border: none; border-radius: 8px; font-size: 16px; cursor: pointer; width: 100%; margin: 10px 0; }\n";
  html += "        .btn:hover { background: #5a6fd8; }\n";
  html += "        .btn-freq { background: #28a745; }\n";
  html += "        .btn-freq:hover { background: #218838; }\n";
  html += "\n";
  html += "        .string-stats { display: grid; grid-template-columns: 1fr 1fr 1fr; gap: 15px; margin: 20px 0; }\n";
  html += "        .stat-box { background: #f8f9fa; padding: 15px; border-radius: 8px; text-align: center; border: 2px solid #e9ecef; }\n";  html += "        .stat-number { font-size: 24px; font-weight: bold; color: #667eea; }\n";
  html += "        .stat-label { font-size: 12px; color: #6c757d; margin-top: 5px; }\n";
  html += "        #currentFreq { font-weight: bold; color: #28a745; }\n";
  html += "    </style>\n";
  html += "</head>\n";
  html += "<body>\n";
  html += "    <div class=\"container\">\n";  html += "        <h1>ESP32 LED String Controller</h1>\n";
  html += "        <div class=\"pattern-display\">\n";
  html += "            <h3>Current String:</h3>\n";
  html += "            <div class=\"pattern-text\" id=\"currentPattern\">" + currentPattern + "</div>\n";
  html += "            <div class=\"string-stats\">\n";
  html += "                <div class=\"stat-box\">\n";
  html += "                    <div class=\"stat-number\" id=\"totalBits\">" + String(currentPattern.length()) + "</div>\n";
  html += "                    <div class=\"stat-label\">Total Bits</div>\n";
  html += "                </div>\n";
  html += "                <div class=\"stat-box\">\n";
  html += "                    <div class=\"stat-number\" id=\"zeroBits\">" + String(this->countBits('0')) + "</div>\n";
  html += "                    <div class=\"stat-label\">LED ON (0)</div>\n";
  html += "                </div>\n";
  html += "                <div class=\"stat-box\">\n";
  html += "                    <div class=\"stat-number\" id=\"oneBits\">" + String(this->countBits('1')) + "</div>\n";
  html += "                    <div class=\"stat-label\">LED OFF (1)</div>\n";
  html += "                </div>\n";
  html += "            </div>\n";
  html += "        </div>\n";        // Frequency info section
  
  html += "        <form onsubmit=\"updatePattern(event)\">\n";
  html += "            <div class=\"input-row\">\n";
  html += "                <div class=\"input-group\">\n";
  html += "                    <label for=\"pattern\">Binary String (0=LED ON, 1=LED OFF):</label>\n";
  html += "                    <input type=\"text\" id=\"pattern\" name=\"pattern\" value=\"" + currentPattern + "\" pattern=\"[01]+\" title=\"Only 0 and 1 allowed\" required>\n";
  html += "                </div>\n";  html += "                <div class=\"input-group\">\n";
  html += "                    <label for=\"frequency\">Frequency (Hz):</label>\n";
  html += "                    <input type=\"number\" id=\"frequency\" name=\"frequency\" value=\"\" min=\"0\" max=\"100000\" placeholder=\"Enter frequency (0=Default)\" onkeypress=\"return event.charCode >= 48 && event.charCode <= 57\">\n";
  html += "                    <div style=\"font-size: 12px; color: #666; margin-top: 5px;\">Current: <span id=\"currentFreq\">" + String(this->useCustomFrequency ? this->customFrequencyHz : 0) + "</span> Hz</div>\n";
  html += "                </div>\n";
  html += "            </div>\n";
  html += "            <div class=\"input-row\">\n";
  html += "                <button type=\"submit\" class=\"btn\">Update String</button>\n";
  html += "                <button type=\"button\" class=\"btn btn-freq\" onclick=\"updateFrequency()\">Set Frequency</button>\n";
  html += "            </div>\n";
  html += "        </form>\n";
  html += "    </div>\n";
  
  // JavaScript
  html += "    <script>\n";
  html += "        function updatePattern(event) {\n";
  html += "            event.preventDefault();\n";
  html += "            const inputString = document.getElementById('pattern').value;\n";
  html += "            if (!/^[01]+$/.test(inputString)) { alert('String can only contain 0 and 1!'); return; }\n";
  html += "            if (inputString.length === 0) { alert('String cannot be empty!'); return; }\n";
  html += "            fetch('/update_pattern', { method: 'POST', headers: { 'Content-Type': 'application/x-www-form-urlencoded' }, body: 'pattern=' + encodeURIComponent(inputString) })\n";
  html += "            .then(response => response.text()).then(data => {\n";
  html += "                if (data === 'OK') { alert('String updated successfully!'); setTimeout(() => location.reload(), 1000); }\n";
  html += "                else { alert('Error: ' + data); }\n";
  html += "            }).catch(error => alert('Connection error: ' + error));\n";
  html += "        }\n";  html += "        function updateFrequency() {\n";
  html += "            const freqInput = document.getElementById('frequency');\n";
  html += "            const frequency = parseInt(freqInput.value) || 0;\n";
  html += "            if (frequency < 0 || frequency > 100000) { alert('Frequency must be 0-100000 Hz!'); return; }\n";
  html += "            if (freqInput.value.trim() === '') { alert('Please enter a frequency value!'); return; }\n";
  html += "            fetch('/update_frequency', { method: 'POST', headers: { 'Content-Type': 'application/x-www-form-urlencoded' }, body: 'frequency=' + frequency })\n";
  html += "            .then(response => response.text()).then(data => {\n";
  html += "                if (data === 'OK') {\n";
  html += "                    const mode = frequency === 0 ? 'Default Software Timer (2Hz - Microsecond precision)' : 'Software Timer (' + frequency + ' Hz - Microsecond precision)';\n";
  html += "                    alert('Frequency updated!\\n' + mode);\n";
  html += "                    // Clear input field and update current display\n";
  html += "                    freqInput.value = '';\n";
  html += "                    freqInput.placeholder = 'Enter new frequency (Current: ' + frequency + ' Hz)';\n";
  html += "                    document.getElementById('currentFreq').textContent = frequency;\n";
  html += "                } else { \n";
  html += "                    alert('Error: ' + data); \n";
  html += "                }\n";
  html += "            }).catch(error => {\n";
  html += "                alert('Connection error: ' + error);\n";
  html += "            });\n";
  html += "        }\n";
  html += "        // Real-time status updates\n";
  html += "        setInterval(() => {\n";  html += "            fetch('/status').then(response => response.json()).then(data => {\n";
  html += "                document.getElementById('currentPattern').textContent = data.pattern;\n";
  html += "                document.getElementById('totalBits').textContent = data.pattern.length;\n";
  html += "                document.getElementById('zeroBits').textContent = (data.pattern.match(/0/g) || []).length;\n";
  html += "                document.getElementById('oneBits').textContent = (data.pattern.match(/1/g) || []).length;\n";
  html += "                // Update current frequency display only, NEVER touch input field\n";
  html += "                const currentFreq = data.useCustomFreq === 'true' ? data.frequency : 0;\n";
  html += "                document.getElementById('currentFreq').textContent = currentFreq;\n";
  html += "            }).catch(error => console.log('Status error:', error));\n";  html += "        }, 1000);\n";
  html += "        // Initialize frequency input placeholder on page load\n";
  html += "        window.addEventListener('load', function() {\n";
  html += "            const currentFreq = document.getElementById('currentFreq').textContent;\n";
  html += "            const freqInput = document.getElementById('frequency');\n";
  html += "            freqInput.placeholder = 'Enter new frequency (Current: ' + currentFreq + ' Hz)';\n";
  html += "        });\n";
  html += "    </script>\n";
  html += "</body>\n";
  html += "</html>";
    return html;
}

void LEDPatternController::registerEndpoints() {
  // Main page
  this->server->on("/", HTTP_GET, [this]() {
    this->handleRoot();
  });
  
  // API to update pattern
  this->server->on("/update_pattern", HTTP_POST, [this]() {
    this->handlePatternUpdate();
  });
  
  // API to update frequency
  this->server->on("/update_frequency", HTTP_POST, [this]() {
    this->handleFrequencyUpdate();
  });
    // API to get status
  this->server->on("/status", HTTP_GET, [this]() {
    String json = "{";
    json += "\"pattern\":\"" + this->currentPattern + "\",";
    json += "\"currentIndex\":" + String(this->currentIndex) + ",";
    json += "\"currentBit\":\"" + String(this->currentPattern.charAt(this->currentIndex)) + "\",";
    json += "\"frequency\":" + String(this->customFrequencyHz) + ",";
    json += "\"useCustomFreq\":" + String(this->useCustomFrequency ? "\"true\"" : "\"false\"");
    json += "}";
    
    this->server->send(200, "application/json", json);
  });
}

void LEDPatternController::handleRoot() {
  String page = generateWebPage();
  this->server->send(200, "text/html", page);
}

void LEDPatternController::handlePatternUpdate() {
  if (this->server->hasArg("pattern")) {
    String newPattern = this->server->arg("pattern");
    
    // Validate pattern (only contains 0 and 1)
    bool valid = true;
    for (int i = 0; i < newPattern.length(); i++) {
      if (newPattern.charAt(i) != '0' && newPattern.charAt(i) != '1') {
        valid = false;
        break;
      }
    }    if (valid && newPattern.length() > 0) {
      Serial.println(" Pattern update: " + this->currentPattern + " -> " + newPattern);
      
      //  INSTANT PATTERN CHANGE
      this->currentPattern = newPattern;
      this->currentIndex = 0;
      this->lastMicros = micros(); // Reset timing immediately
      
      Serial.println("✅ Pattern updated instantly: " + newPattern);
      this->server->send(200, "text/plain", "OK");
    } else {
      this->server->send(400, "text/plain", "Invalid pattern");
    }
  } else {
    this->server->send(400, "text/plain", "Missing pattern parameter");
  }
}

void LEDPatternController::handleFrequencyUpdate() {
  if (this->server->hasArg("frequency")) {
    String freqStr = this->server->arg("frequency");
    unsigned long frequency = freqStr.toInt();
    
    Serial.println(" Frequency update request: " + String(frequency) + " Hz");
    
    if (frequency >= 1 && frequency <= 100000) { 
      // Valid frequency range: 1Hz to 100kHz
      this->setCustomFrequency(frequency);
      Serial.println(" Frequency set to: " + String(frequency) + " Hz");
      this->server->send(200, "text/plain", "OK");    } else if (frequency == 0) {
      // Switch back to default timing - Use 2Hz software timer
      Serial.println(" Switching to default mode: 2Hz software timer");
      this->setCustomFrequency(0); // This will set up 2Hz software timer and mark as default mode
      
      Serial.println(" Reset to default mode (2Hz software timer - 500ms per bit)");
      this->server->send(200, "text/plain", "OK");
    } else {
      Serial.println(" Invalid frequency: " + String(frequency));
      this->server->send(400, "text/plain", "Invalid frequency! Range: 1-100000 Hz or 0 for default");
    }
  } else {
    Serial.println(" Missing frequency parameter");
    this->server->send(400, "text/plain", "Missing frequency parameter");
  }
}

// Getter methods
unsigned long LEDPatternController::getCustomFrequency() const {
  return this->customFrequencyHz;
}

void LEDPatternController::resetTimingSync() {
  // Reset all timing variables to ensure pattern synchronization
  this->lastPatternTime = millis();
  this->lastMicros = micros();
  // DO NOT reset currentIndex here - it should be managed by the calling function
  
  Serial.println(" Timing synchronization reset");
}

void LEDPatternController::handleFirmwareUpload() {
  this->server->send(200, "text/plain", "Firmware upload complete. Device restarting...");
  Serial.println("Firmware upload completed successfully");
  Serial.println("Restarting device...");
  delay(1000);
  ESP.restart();
}

void LEDPatternController::handleFirmwareUpdate() {
  HTTPUpload& upload = this->server->upload();
    if (upload.status == UPLOAD_FILE_START) {
    Serial.printf("Firmware update start: %s\n", upload.filename.c_str());
    if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
      Serial.println("Cannot start update");
      Update.printError(Serial);
    }
  } 
  else if (upload.status == UPLOAD_FILE_WRITE) {
    if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
      Serial.println("Write failed");
      Update.printError(Serial);
    }
  } 
  else if (upload.status == UPLOAD_FILE_END) {    if (Update.end(true)) {
      Serial.printf("Update success: %u bytes\n", upload.totalSize);
    } else {
      Serial.println("Update failed");
      Update.printError(Serial);
    }
  }
}
