/**
 * @file M5Card_Wifi_KeyBoard_Setup.ino
 * @author Aurélio Avanzi
 * @brief https://github.com/cyberwisk/M5Card_Wifi_KeyBoard_Setup/tree/main/M5Card_Wifi_KeyBoard_Setup
 * @version Apha 0.3
 * @date 2024-01-30
 *
 * @Hardwares: M5Cardputer - https://docs.m5stack.com/en/core/Cardputer
 * @Dependent Librarys:
 * M5Cardputer: https://github.com/m5stack/M5Cardputer
 * WiFi: https://github.com/espressif/arduino-esp32/tree/master/libraries/WiFi
 **/

#ifndef M5CARD_WIFI_SETUP_H
#define M5CARD_WIFI_SETUP_H

#include <M5Cardputer.h>
#include <WiFi.h>
#include <Preferences.h>

class M5CardWifiSetup {
public:
    M5CardWifiSetup();
    void displayWiFiInfo();
    void connectToWiFi();
    String scanAndDisplayNetworks();
	private:
    String inputText(const String& prompt, int x, int y);
};

#endif  // M5CARD_WIFI_SETUP_H
 
