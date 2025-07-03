#ifndef LED_PATTERN_CONTROLLER_H
#define LED_PATTERN_CONTROLLER_H

#include <WebServer.h>
#include <WiFi.h>
#include <Update.h>

class LEDPatternController {
  private:
    WebServer* server;
    String currentPattern;
    int currentIndex;
    unsigned long lastPatternTime;
    unsigned long patternInterval;
    int ledPin;
    // VLC and frequency control members
    unsigned long customFrequencyHz;
    bool useCustomFrequency;
    // Software timer members
    unsigned long lastMicros; // Microsecond timing for high frequency mode
    bool softwareTimerActive;
      void setupWebPages();
    String generateWebPage();
    
  public:
    LEDPatternController(WebServer* server, int ledPin, unsigned long interval = 1000);
    void setup();
    void registerEndpoints();
    void handleLoop();
    void handlePatternUpdate();
    void handleRoot();
    void executePattern();
    // Frequency control methods
    void setCustomFrequency(unsigned long frequencyHz);
    unsigned long getCustomFrequency() const;
    void handleFrequencyUpdate();
    // Utility methods
    int countBits(char bit);
    void resetTimingSync(); // Reset timing synchronization for pattern changes
    
    // Firmware update methods
    void handleFirmwareUpload();
    void handleFirmwareUpdate();
};

#endif
