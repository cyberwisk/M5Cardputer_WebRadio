/**
 * @file M5Cardputer_WebRadio.ino
 * @author Aurélio Avanzi
 * @brief https://github.com/cyberwisk/M5Cardputer_WebRadio
 * @version Beta 1.1
 * @date 2023-12-12
 *
 * @Hardwares: M5Cardputer
 * @Platform Version: Arduino M5Stack Board Manager v2.0.7
 * @Dependent Library:
 * M5GFX: https://github.com/m5stack/M5GFX
 * M5Unified: https://github.com/m5stack/M5Unified
 * M5Cardputer: https://github.com/m5stack/M5Cardputer
 * Preferences: https://github.com/espressif/arduino-esp32/tree/master/libraries/Preferences
 **/

#include "Audio.h" // ESP32-audioI2S Versão 3.0.13
#include "M5Cardputer.h" // Versão 3.2.0
#include "CardWifiSetup.h"

Audio audio;
bool isPlaying = false;
unsigned long lastScrollUpdate = 0;
int xOffset = 0; // Posição inicial do texto
String currentTitle = ""; // Nome da música atual

void setup() {
    Serial.begin(115200);
    delay(2000);

    M5Cardputer.begin();
    M5Cardputer.Display.setRotation(1);
    M5Cardputer.Display.fillScreen(BLACK);
    M5Cardputer.Display.setFont(&fonts::FreeMonoOblique9pt7b);
    M5Cardputer.Display.println("Iniciando...");

    // Conectar ao WiFi
    connectToWiFi();
    Serial.println("WiFi conectado!");
    M5Cardputer.Display.setCursor(0, 0);
    M5Cardputer.Display.println("WiFi OK");

    // Configurar áudio
    audio.setPinout(41, 43, 42);
    audio.setVolume(6); // Volume fixo

    // Tentativa de conectar ao stream SEM SSL
    bool connected = audio.connecttohost("http://radio.morcegaofm.com.br/morcegao32");
    //bool connected = audio.connecttohost("http://server03.srvsh.com.br:6678");

    if (connected) {
        Serial.println("Streaming iniciado!");
        isPlaying = true;
        M5Cardputer.Display.setCursor(0, 55);
        M5Cardputer.Display.println("Reproduzindo...");
    } else {
        Serial.println("Falha ao conectar ao stream!");
        M5Cardputer.Display.setCursor(0, 55);
        M5Cardputer.Display.println("Erro!");
    }
}

void loop() {
    M5Cardputer.update();
    if (isPlaying) {
        audio.loop();
        Serial.printf("Audio isRunning: %s | Heap livre: %d bytes\n", 
                      audio.isRunning() ? "YES" : "NO", ESP.getFreeHeap());
    }

    // Atualiza a rolagem do título dentro do loop principal
    if (millis() - lastScrollUpdate > 300 && currentTitle.length() > 20) {
        lastScrollUpdate = millis();
        xOffset++;
        if (xOffset > currentTitle.length() * 6 - M5Cardputer.Display.width()) {
            xOffset = 0; // Reinicia a posição do texto
        }
        audio_showstreamtitle(currentTitle.c_str());
    }

    vTaskDelay(pdMS_TO_TICKS(5)); 
}

// Exibe o nome da estação de rádio
void audio_showstation(const char *showstation) {
    if (!showstation || *showstation == '\0') {
        Serial.println("Erro: Ponteiro showstation é NULL ou vazio!");
        return;
    }

    char limitedInfo[21];  // 20 caracteres + 1 para o terminador nulo
    strncpy(limitedInfo, showstation, 20);  // Copia apenas os primeiros 20 caracteres
    limitedInfo[20] = '\0';  // Garante que a string termine corretamente
    
    M5Cardputer.Display.fillRect(0, 15, M5Cardputer.Display.width(), 15, TFT_BLACK);
    M5Cardputer.Display.setCursor(0, 15);
    M5Cardputer.Display.setTextColor(WHITE);
    M5Cardputer.Display.drawString(limitedInfo, 0, 15);
}

// Atualiza o nome da música e reinicia a rolagem ao mudar
void audio_showstreamtitle(const char *info) {
    if (!info || *info == '\0') { 
        Serial.println("Erro: Ponteiro info é NULL ou vazio!");
        return;
    }

    char titleBuffer[100];  // Buffer fixo para evitar alocações dinâmicas
    strncpy(titleBuffer, info, sizeof(titleBuffer) - 1);
    titleBuffer[sizeof(titleBuffer) - 1] = '\0';  // Garante que a string termine corretamente

    currentTitle = String(titleBuffer); // Armazena novo título sem usar diretamente String(info)
    xOffset = 0; // Reinicia a rolagem

    M5Cardputer.Display.fillRect(0, 33, M5Cardputer.Display.width(), 18, BLACK);
    M5Cardputer.Display.setCursor(-xOffset, 33);
    M5Cardputer.Display.setTextColor(WHITE);
    M5Cardputer.Display.drawString(currentTitle, 0, 33);
    M5Cardputer.Display.fillRect(0, 50, 240, 1, TFT_RED);
}
