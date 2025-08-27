 /* ***************** POR FAVOR MANTENHA OS CREDITOS **************************
 * @file CardWifiSetup.h
 * @author Aurélio Avanzi (Cyberwisk)
 * @brief https://github.com/cyberwisk/M5Card_Wifi_KeyBoard_Setup
 * @version Beta 1.1
 * @date 2024-02-18
 * @modificado 1015-05-25
 *
 * @Hardwares: M5Cardputer - https://docs.m5stack.com/en/core/Cardputer
 * @Dependent Librarys:
 * M5Cardputer: https://github.com/m5stack/M5Cardputer
 * WiFi: https://github.com/espressif/arduino-esp32
 ******************** POR FAVOR MANTENHA OS CREDITOS *************************
 * */

#include <WiFi.h>
#include <Preferences.h>
#include <esp_wifi.h>
#include <vector>

// Definições de constantes
#define NVS_NAMESPACE "M5_settings"
#define NVS_SSID_KEY "wifi_ssid"
#define NVS_PASS_KEY "wifi_pass"
#define WIFI_TIMEOUT 20000  // 20 segundos
#define MIN_WIFI_RSSI -80   // Sinal mínimo aceitável
#define MAX_NETWORKS 10     // Máximo de redes exibidas

String CFG_WIFI_SSID;
String CFG_WIFI_PASS;
Preferences preferences;

// Estrutura para armazenar informações das redes
struct WiFiNetwork {
    String ssid;
    int32_t rssi;
    wifi_auth_mode_t encryption;
};
std::vector<WiFiNetwork> networks;

// Função auxiliar para calcular hash das credenciais
uint32_t calculateHash(const String& str) {
    uint32_t hash = 5381;
    for (char c : str) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash;
}

// Função melhorada para entrada de texto
String inputText(const String& prompt, int x, int y, bool isPassword = false) {
    String data = "> ";
    String displayData = "> ";
    
    M5Cardputer.Display.setRotation(1);
    M5Cardputer.Display.setTextScroll(true);
    M5Cardputer.Display.drawString(prompt, x, y);
    
    while (true) {
        M5Cardputer.update();
        if (M5Cardputer.Keyboard.isChange()) {
            if (M5Cardputer.Keyboard.isPressed()) {
                //M5Cardputer.Speaker.tone(4000, 20);
                Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();
                
                for (auto i : status.word) {
                    data += i;
                    //displayData += (isPassword ? '*' : i);
                    displayData += i;
                }
                
                if (status.del && data.length() > 2) {
                    data.remove(data.length() - 1);
                    displayData.remove(displayData.length() - 1);
                }
                
                if (status.enter) {
                    data.remove(0, 2);
                    M5Cardputer.Display.println(displayData);
                    return data;
                }
                
                M5Cardputer.Display.fillRect(0, y - 4, M5Cardputer.Display.width(), 25, BLACK);
                M5Cardputer.Display.drawString(displayData, 4, y);
            }
        }
        delay(10);  // Reduzido para melhor responsividade
    }
}

void displayWiFiInfo() {
    M5Cardputer.Display.fillRect(0, 20, 240, 135, BLACK);
    M5Cardputer.Display.setCursor(1, 1);
    M5Cardputer.Display.drawString("WiFi conectado", 35, 1);
    M5Cardputer.Display.drawString("SSID: " + WiFi.SSID(), 1, 18);
    M5Cardputer.Display.drawString("IP: " + WiFi.localIP().toString(), 1, 33);
    int8_t rssi = WiFi.RSSI();
    M5Cardputer.Display.drawString("RSSI: " + String(rssi) + " dBm", 1, 48);
    delay(2000);
    M5Cardputer.Display.fillRect(0, 0, 240, 135, BLACK);
}

String getSecurityString(wifi_auth_mode_t encType) {
    switch(encType) {
        case WIFI_AUTH_OPEN: return "Aberta";
        case WIFI_AUTH_WEP: return "WEP";
        case WIFI_AUTH_WPA_PSK: return "WPA";
        case WIFI_AUTH_WPA2_PSK: return "WPA2";
        case WIFI_AUTH_WPA_WPA2_PSK: return "WPA/WPA2";
        default: return "---";
    }
}

String scanAndDisplayNetworks() {
    // Configura scan assíncrono
    WiFi.scanDelete();
    WiFi.scanNetworks(true);
    
    M5Cardputer.Display.clear();
    M5Cardputer.Display.drawString("Procurando redes...", 1, 1);
    
    // Aguarda resultado do scan
    int16_t scanResult;
    do {
        scanResult = WiFi.scanComplete();
        delay(100);
    } while(scanResult == WIFI_SCAN_RUNNING);
    
    if (scanResult == 0) {
        M5Cardputer.Display.drawString("Nenhuma rede encontrada.", 1, 15);
        delay(2000);
        return "";
    }
    
    // Armazena e ordena as redes
    networks.clear();
    for (int i = 0; i < scanResult && i < MAX_NETWORKS; i++) {
        if (WiFi.RSSI(i) >= MIN_WIFI_RSSI) {
            networks.push_back({
                WiFi.SSID(i),
                WiFi.RSSI(i),
                WiFi.encryptionType(i)
            });
        }
    }
    
    // Ordena por força do sinal
    std::sort(networks.begin(), networks.end(),
             [](const WiFiNetwork& a, const WiFiNetwork& b) {
                 return a.rssi > b.rssi;
             });
    
    M5Cardputer.Display.clear();
    M5Cardputer.Display.drawString("Redes disponiveis:", 1, 1);
    
    int selectedNetwork = 0;
    while (true) {
        for (size_t i = 0; i < networks.size(); i++) {
            String prefix = (i == selectedNetwork) ? "-> " : "   ";
            String quality = String(networks[i].rssi) + "dBm";
            String secure = (networks[i].encryption != WIFI_AUTH_OPEN) ? " *" : "";
            
            M5Cardputer.Display.fillRect(1, 18 + i * 18, 240, 18, BLACK);
            M5Cardputer.Display.drawString(prefix + networks[i].ssid + 
                                         " (" + quality + ")" + secure, 
                                         1, 18 + i * 18);
        }
        
        M5Cardputer.Display.drawString("Selecionar ENTER:OK", 1, 108);
        M5Cardputer.update();
        
        if (M5Cardputer.Keyboard.isChange()) {
            if (M5Cardputer.Keyboard.isPressed()) {
                //M5Cardputer.Speaker.tone(3000, 20);
                
                if (M5Cardputer.Keyboard.isKeyPressed(';') && selectedNetwork > 0) {
                    selectedNetwork--;
                }
                if (M5Cardputer.Keyboard.isKeyPressed('.') && 
                    selectedNetwork < networks.size() - 1) {
                    selectedNetwork++;
                }
                if (M5Cardputer.Keyboard.isKeyPressed(KEY_ENTER)) {
                    return networks[selectedNetwork].ssid;
                }
            }
        }
        delay(10);
    }
}

void connectToWiFi() {
    // Configura modo WiFi e economia de energia
    WiFi.mode(WIFI_STA);
    esp_wifi_set_ps(WIFI_PS_MAX_MODEM);
    
    // Tenta recuperar credenciais salvas
    preferences.begin(NVS_NAMESPACE, true);
    CFG_WIFI_SSID = preferences.getString(NVS_SSID_KEY, "");
    CFG_WIFI_PASS = preferences.getString(NVS_PASS_KEY, "");
    uint32_t stored_ssid_hash = preferences.getUInt("ssid_hash", 0);
    uint32_t stored_pass_hash = preferences.getUInt("pass_hash", 0);
    preferences.end();
    
    // Verifica hash das credenciais
    bool validCredentials = !CFG_WIFI_SSID.isEmpty() && 
                          (calculateHash(CFG_WIFI_SSID) == stored_ssid_hash) &&
                          (calculateHash(CFG_WIFI_PASS) == stored_pass_hash);
    
    if (validCredentials) {
        WiFi.begin(CFG_WIFI_SSID.c_str(), CFG_WIFI_PASS.c_str());
        
        unsigned long startTime = millis();
        M5Cardputer.Display.print("Conectando");
        
        while (millis() - startTime < WIFI_TIMEOUT) {
            M5Cardputer.update();
            
            if (M5Cardputer.BtnA.isPressed()) {
                //M5Cardputer.Speaker.tone(7000, 1000);
                preferences.begin(NVS_NAMESPACE, false);
                preferences.clear();
                preferences.end();
                M5Cardputer.Display.clear();
                M5Cardputer.Display.drawString("Memoria apagada.", 1, 60);
                delay(1000);
                ESP.restart();
                return;
            }
            
            if (WiFi.status() == WL_CONNECTED) {
                displayWiFiInfo();
                return;
            }
            
            M5Cardputer.Display.print(".");
            delay(50);
        }
    }
    
    // Se não conectou, inicia processo de configuração
    M5Cardputer.Display.clear();
    M5Cardputer.Display.drawString("Configuracao WiFi", 1, 1);
    
    CFG_WIFI_SSID = scanAndDisplayNetworks();
    if (CFG_WIFI_SSID.isEmpty()) {
        return;
    }
    
    M5Cardputer.Display.clear();
    M5Cardputer.Display.drawString("SSID: " + CFG_WIFI_SSID, 1, 20);
    M5Cardputer.Display.drawString("Digite a senha:", 1, 38);
    CFG_WIFI_PASS = inputText("> ", 4, M5Cardputer.Display.height() - 24, true);
    
    // Salva credenciais com hash
    preferences.begin(NVS_NAMESPACE, false);
    preferences.putString(NVS_SSID_KEY, CFG_WIFI_SSID);
    preferences.putString(NVS_PASS_KEY, CFG_WIFI_PASS);
    preferences.putUInt("ssid_hash", calculateHash(CFG_WIFI_SSID));
    preferences.putUInt("pass_hash", calculateHash(CFG_WIFI_PASS));
    preferences.end();
    
    M5Cardputer.Display.clear();
    M5Cardputer.Display.drawString("Credenciais salvas.", 1, 60);
    
    WiFi.begin(CFG_WIFI_SSID.c_str(), CFG_WIFI_PASS.c_str());
    delay(300);
    displayWiFiInfo();
}
