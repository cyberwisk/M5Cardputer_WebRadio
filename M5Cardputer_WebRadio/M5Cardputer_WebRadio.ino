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

//Display: Tela TFT de 1.14 polegadas com resolução de 135x240 pixels.
#include "M5Cardputer.h"
#include "CardWifiSetup.h"
#include <Audio.h> //ESP32-audioI2S vesão 
#include <Adafruit_NeoPixel.h>
Adafruit_NeoPixel led(1, 21, NEO_GRB + NEO_KHZ800);

#define MAX_STATIONS 20
#define MAX_NAME_LENGTH 30
#define MAX_URL_LENGTH 100
#define I2S_BCK 41
#define I2S_WS 43
#define I2S_DOUT 42

// FFT Constants
#define FFT_SIZE 256
#define WAVE_SIZE 320
static uint16_t prev_y[(FFT_SIZE / 2)+1];
static uint16_t peak_y[(FFT_SIZE / 2)+1];
static int header_height = 51; // Altura do FFT
static bool fft_enabled = false;  // Flag para habilitar/desabilitar FFT
static bool fftSimON = true; //Liga a simulação do FFT

Audio audio;

//const long interval = 100;
unsigned long lastUpdate = 0;

// Classe FFT simplificada
class fft_t {
public:
  fft_t() {
    for (int i = 0; i < FFT_SIZE; i++) {
      _data[i] = 0;
    }
  }

  void exec(const int16_t* in) {
    // Simula FFT para demonstração
    if (fftSimON) {
      for (int i = 0; i < FFT_SIZE; i++) {
        _data[i] = abs(in[i]);
      }
    }
  }

  uint32_t get(size_t index) {
    if (index < FFT_SIZE) {
      return _data[index];
    }
    return 0;
  }

private:
  uint32_t _data[FFT_SIZE];
};

static fft_t fft;
static int16_t raw_data[WAVE_SIZE * 2];

// Função para obter cor de fundo baseada na posição Y
static uint32_t bgcolor(int y) {
  auto h = M5Cardputer.Display.height();
  auto dh = h - header_height;
  int v = ((h - y) << 5) / dh;
  if (dh > header_height) {
    int v2 = ((h - y - 1) << 5) / dh;
    if ((v >> 2) != (v2 >> 2)) {
      return 0x666666u;
    }
  }
  return M5Cardputer.Display.color888(v + 2, v, v + 6);
}

struct RadioStation {
  char name[MAX_NAME_LENGTH];
  char url[MAX_URL_LENGTH];
};

const PROGMEM RadioStation defaultStations[] = {
  {"Radio Porao", "https://server03.stmsg.com.br:6678/stream"},
  {"Morcegao FM", "https://radio.morcegaofm.com.br/morcegao32"},
  {"Mundo Livre", "https://up-rcr.webnow.com.br/mundolivre.mp3"},
  {"Radio Mundo do Rock","https://servidor34.brlogic.com:8014/live"},
};

RadioStation stations[MAX_STATIONS];
size_t numStations = 0;
size_t curStation = 3; //qual radio iniciar
uint16_t curVolume = 115;

// Controle de debounce
unsigned long lastButtonPress = 0;
const unsigned long DEBOUNCE_DELAY = 200;

// Funções FFT
void setupFFT() {
  if (!fft_enabled) return;
  
  // Inicializa os arrays de FFT
  for (int x = 0; x < (FFT_SIZE / 2) + 1; ++x) {
    prev_y[x] = INT16_MAX;
    peak_y[x] = INT16_MAX;
  }

  // Desenha o fundo gradiente
  int display_height = M5Cardputer.Display.height();
  for (int y = header_height; y < display_height; ++y) {
    M5Cardputer.Display.drawFastHLine(0, y, M5Cardputer.Display.width(), bgcolor(y));
  }
}

void updateFFT() {
  if (!fft_enabled) return;

  static unsigned long lastFFTUpdate = 0;
  if (millis() - lastFFTUpdate < 50) return; // Atualiza a cada 50ms
  lastFFTUpdate = millis();

  // Preencha raw_data com amostras de áudio
  for (int i = 0; i < WAVE_SIZE * 2; i++) {
    raw_data[i] = random(-32000, 32000);
  }

  // Executa FFT
  fft.exec(raw_data);

  // Parâmetros para desenho
  size_t bw = M5Cardputer.Display.width() / 30;
  if (bw < 3) bw = 3;
  int32_t dsp_height = M5Cardputer.Display.height();
  int32_t fft_height = dsp_height - header_height - 1;
  size_t xe = M5Cardputer.Display.width() / bw;
  if (xe > (FFT_SIZE / 2)) xe = (FFT_SIZE / 2);

  uint32_t bar_color[2] = {0x000033u, 0x99AAFFu};

  M5Cardputer.Display.startWrite();
  
  for (size_t bx = 0; bx <= xe; ++bx) {
    size_t x = bx * bw;
    int32_t f = fft.get(bx) * 3; //intencidade 
    int32_t y = (f * fft_height) >> 17; //Escala da FFT original =18
    if (y > fft_height) y = fft_height;
    y = dsp_height - y;
    int32_t py = prev_y[bx];
    if (y != py) {
      M5Cardputer.Display.fillRect(x, y, bw - 1, py - y, bar_color[(y < py)]);
      prev_y[bx] = y;
    }
    py = peak_y[bx] + ((peak_y[bx] - y) > 5 ? 2 : 1);
    if (py < y) {
      M5Cardputer.Display.writeFastHLine(x, py - 1, bw - 1, bgcolor(py - 1));
    } else {
      py = y - 1;
    }
    if (peak_y[bx] != py) {
      peak_y[bx] = py;
      M5Cardputer.Display.writeFastHLine(x, py, bw - 1, TFT_WHITE);
    }
  }
  
  M5Cardputer.Display.endWrite();
}

void toggleFFT() {
  fft_enabled = !fft_enabled;
  M5Cardputer.Display.fillRect(0, 51, 240, 89, TFT_BLACK);
  if (fft_enabled) {
    setupFFT();
  }
}

void updateBatteryDisplay(unsigned long updateInterval) {
  static unsigned long lastUpdate = 0;

  if (millis() - lastUpdate >= updateInterval) {
    lastUpdate = millis();
    
    int batteryLevel = M5.Power.getBatteryLevel();
    uint16_t batteryColor = batteryLevel < 30 ? TFT_RED : TFT_GREEN;

    M5Cardputer.Display.fillRect(215, 5, 40, 12, TFT_BLACK); 

    M5Cardputer.Display.fillRect(215, 5, 20, 10, TFT_DARKGREY);
    M5Cardputer.Display.fillRect(235, 7, 3, 6, TFT_DARKGREY);
    M5Cardputer.Display.fillRect(217, 7, (batteryLevel * 16) / 100, 6, batteryColor);
    
    //M5Cardputer.Display.drawString(batteryLevel,215 + 10,5 + 5,&fonts::Font0);
    
    //char percentageStr[4];
    //snprintf(percentageStr, sizeof(percentageStr), "%d%%", batteryLevel);

    //M5Cardputer.Display.setTextColor(TFT_BLACK);
    //M5Cardputer.Display.setTextDatum(MC_DATUM);
    //M5Cardputer.Display.drawString(percentageStr,215 + 10,5 + 5,&fonts::Font0);
    //M5Cardputer.Display.drawString("Fonte maior", 10, 40, 2);
    // Restaura a fonte original
    //M5Cardputer.Display.setFont(&fonts::FreeMonoOblique9pt7b);
  }
}

void loadDefaultStations() {
  numStations = std::min(sizeof(defaultStations)/sizeof(defaultStations[0]), static_cast<size_t>(MAX_STATIONS));
  memcpy(stations, defaultStations, sizeof(RadioStation) * numStations);
}

void mergeRadioStations() {
  if (!SD.begin()) {
    led.setPixelColor(0, led.Color(255, 0, 0));
    led.show();
    M5Cardputer.Display.drawString("/station_list.txt ", 20, 30);
    M5Cardputer.Display.drawString("NAO Encontrado no SD", 20, 50);
    delay(4000);
    loadDefaultStations();
    M5Cardputer.Display.fillScreen(BLACK);
    return;
  }

  File file = SD.open("/station_list.txt");
  if (!file) {
    loadDefaultStations();
    return;
  }

  numStations = 0;
  
  String line;
  while (file.available() && numStations < MAX_STATIONS) {
    line = file.readStringUntil('\n');
    int commaIndex = line.indexOf(',');
    
    if (commaIndex > 0) {
      String name = line.substring(0, commaIndex);
      String url = line.substring(commaIndex + 1);
      
      name.trim();
      url.trim();
      
      if (name.length() > 0 && url.length() > 0) {
        strncpy(stations[numStations].name, name.c_str(), MAX_NAME_LENGTH - 1);
        strncpy(stations[numStations].url, url.c_str(), MAX_URL_LENGTH - 1);
        stations[numStations].name[MAX_NAME_LENGTH - 1] = '\0';
        stations[numStations].url[MAX_URL_LENGTH - 1] = '\0';
        numStations++;
      }
    }
  }

  file.close();
  if (numStations == 0) {
    loadDefaultStations();
  }
    led.setPixelColor(0, led.Color(0, 0, 0));
    led.show();
}

void showStation() {
  fftSimON = false;
  M5Cardputer.Display.fillRect(0, 15, 240, 35, TFT_BLACK);
  M5Cardputer.Display.drawString(stations[curStation].name, 0, 15);
  showVolume();
}

void audio_id3data(const char *info){M5Cardputer.Display.drawString(info, 0, 33);}

void Playfile() {
  led.setPixelColor(0, led.Color(255, 0, 0));
  led.show();
  audio.stopSong();
    
    String url = stations[curStation].url; // Armazena a URL para facilitar o acesso
    
    if (url.indexOf("http") != -1) { 
        audio.connecttohost(stations[curStation].url);
    } 
    else if (url.indexOf("/mp3") != -1) {
        M5Cardputer.Display.drawString("Play MP3 no SD /mp3    ", 0, 15);
        delay(4000);
        audio.connecttoFS(SD,stations[curStation].url);
    } 
    else {
        audio.connecttospeech("Trabalhe em quanto os outros dormem, e você ficará com sono durante o dia.", "pt");
    }
  showStation();
}


void volumeUp() {
  if (curVolume < 255) {
    curVolume = std::min(static_cast<uint16_t>(curVolume + 10), static_cast<uint16_t>(255));
    audio.setVolume(map(curVolume, 0, 255, 0, 21));
    showVolume();
  }
}

void volumeDown() {
  if (curVolume > 0) {
    curVolume = std::max(static_cast<uint16_t>(curVolume - 10), static_cast<uint16_t>(0));
    audio.setVolume(map(curVolume, 0, 255, 0, 21));
    showVolume();
  }
}

bool isMuted = false;
uint16_t prevVolume = 0; // Guarda o volume antes do mute

void volumeMute() {
  if (!isMuted) {
    prevVolume = curVolume;
    curVolume = 0;
    isMuted = true;
  } else {
    curVolume = prevVolume;
    isMuted = false;
  }
  audio.setVolume(map(curVolume, 0, 255, 0, 21));
  showVolume();
}

void showVolume() {
  static uint8_t lastVolume = 0; // Rastrear o último volume
  uint8_t currentVolume = curVolume; // Usar variável global

  if (currentVolume != lastVolume) {
    lastVolume = currentVolume;

    int barHeight = 4; // Altura da barra
    M5Cardputer.Display.fillRect(0, 6, 200, 6, TFT_BLACK);
    int barWidth = map(currentVolume, 0, 200, 0, M5Cardputer.Display.width());
    if (barWidth < 200) {
      M5Cardputer.Display.fillRect(0, 6, barWidth, barHeight, 0xAAFFAA); // Verde claro
    }

    // Opcional: Exibe valor numérico (ajuste as coordenadas)
    //M5Cardputer.Display.setTextColor(TFT_WHITE, TFT_BLACK);
    //M5Cardputer.Display.setCursor(5, 0); // Posição do texto
    //M5Cardputer.Display.printf("Vol: %d%%", (currentVolume * 100) / 255);
  }
}

void stationUp() {
  if (numStations > 0) {
    curStation = (curStation + 1) % numStations; 
    audio.stopSong();
    //audio.connecttohost(stations[curStation].url);
    Playfile();
    showStation();
  }
  showVolume();
}

void stationDown() {
  if (numStations > 0) {
    curStation = (curStation - 1 + numStations) % numStations; 
    audio.stopSong();
    //audio.connecttohost(stations[curStation].url);
    Playfile();
    showStation();
  }
  showVolume();
}

void setup() {
  auto cfg = M5.config();
  auto spk_cfg = M5Cardputer.Speaker.config();
    /// Increasing the sample_rate will improve the sound quality instead of increasing the CPU load.
    spk_cfg.sample_rate = 128000; // default:64000 (64kHz)  e.g. 48000 , 50000 , 80000 , 96000 , 100000 , 128000 , 144000 , 192000 , 200000
    spk_cfg.task_pinned_core = APP_CPU_NUM;
    M5Cardputer.Speaker.config(spk_cfg);
  //M5Cardputer.Speaker.setVolume(255);
  
  M5Cardputer.begin(cfg, true);

  led.begin();
  led.setBrightness(255);  // Brilho (0-255)
  led.show();  // Inicializa apagado

  M5Cardputer.Display.setRotation(1);
  M5Cardputer.Display.setFont(&fonts::FreeMonoOblique9pt7b);
 
  connectToWiFi();
  
  audio.setPinout(I2S_BCK, I2S_WS, I2S_DOUT);
  audio.setVolume(map(curVolume, 0, 255, 0, 21));
  audio.setBalance(0);
  
  M5Cardputer.Display.fillScreen(BLACK);  
  
  audio.stopSong();
  mergeRadioStations();
  //audio.connecttohost(stations[curStation].url);
  Playfile();
  showStation();
  toggleFFT();
}

void loop() {
  audio.loop();
  M5Cardputer.update();
  updateBatteryDisplay(5000);

  if (M5Cardputer.Keyboard.isChange() && (millis() - lastButtonPress > DEBOUNCE_DELAY)) {
      //M5Cardputer.Speaker.tone(6000, 10);
    led.setPixelColor(0, led.Color(120, 0, 255));
    led.show();

    if (M5Cardputer.Keyboard.isKeyPressed(';')) volumeUp();
    else if (M5Cardputer.Keyboard.isKeyPressed('.')) volumeDown();
    else if (M5Cardputer.Keyboard.isKeyPressed('m')) volumeMute();
    else if (M5Cardputer.Keyboard.isKeyPressed('/')) stationUp();
    else if (M5Cardputer.Keyboard.isKeyPressed(',')) stationDown();
    else if (M5Cardputer.Keyboard.isKeyPressed('r')) {
      audio.stopSong();
      audio.connecttohost(stations[curStation].url);
      //Playfile();
    }
    else if (M5Cardputer.Keyboard.isKeyPressed('p')) {
      M5Cardputer.Display.fillRect(0, 15, 240, 49, TFT_BLACK);  
      M5Cardputer.Display.drawString("Los Santos Rock", 0, 15);
      audio.stopSong();
      audio.connecttoFS(SD,"/mp3/Los Santos Rock Radio.mp3");
    }
    else if (M5Cardputer.Keyboard.isKeyPressed('o')) {
      M5Cardputer.Display.fillRect(0, 15, 240, 49, TFT_BLACK);  
      M5Cardputer.Display.drawString("PlayFile", 0, 15);
      Playfile();
    }
    else if (M5Cardputer.Keyboard.isKeyPressed('s')) {
      audio.stopSong();
      audio.connecttospeech("Trabalhe em quanto os outros dormem, e você ficará com sono durante o dia.", "pt");
    }
    else if (M5Cardputer.Keyboard.isKeyPressed('f')) {
      toggleFFT();  //tecla 'f' para ativar/desativar FFT
    }
      else if (M5Cardputer.Keyboard.isKeyPressed('f')) {
      toggleFFT();  //tecla 'f' para ativar/desativar FFT
    }
    
    lastButtonPress = millis();
      //char key = M5Cardputer.Keyboard.read();  // Lê a tecla pressionada
      //M5Cardputer.Display.drawString(String(key), 0, 60);  // Converte char para String e exibe na tela
   }
  
  if (fft_enabled) {
    updateFFT();
  } else {
       // M5Cardputer.Display.drawString(strstr(audio_info(info), "HTTP/"), 0, 51);
  }
  
  delay(1);
    led.setPixelColor(0, led.Color(0, 0, 0)); // Azul
    led.show();  // Inicializa apagado  M5.Leds.show();  // Atualiza o LED
}

//void audio_info(const char *info){
//    if (info && strstr(info, "HTTP/1.1 200 OK") == nullptr) {
//        M5Cardputer.Display.fillRect(0, 15, 240, 15, TFT_BLACK);
//        M5Cardputer.Display.drawString(strstr(info, "HTTP/"), 0, 15);
//    }
//}

void audio_showstation(const char *showstation) {
    if (showstation && *showstation) { // Verifica se a string não é nula e não está vazia
        char limitedInfo[241];  // 240 caracteres + 1 para o terminador nulo
        strncpy(limitedInfo, showstation, 24);  // Copia apenas os primeiros 240 caracteres
        limitedInfo[24] = '\0';  // Garante que a string termine corretamente
        M5Cardputer.Display.fillRect(0, 15, 240, 15, TFT_BLACK);
        M5Cardputer.Display.drawString(limitedInfo, 0, 15);
       fftSimON = true;
    }
}

void audio_showstreamtitle(const char *info) {
  static int xOffset = 0;                  // Posição horizontal do texto
  static unsigned long lastUpdate = 0;     // Controle de tempo
  const int updateInterval = 100;          // Velocidade da rolagem (ms)

  if (info && *info) { 
    int textWidth = M5Cardputer.Display.textWidth(info);  // Largura do texto

    // Se o texto for maior que a tela, aplicamos a rolagem
    if (textWidth > 21) {
      if (millis() - lastUpdate > updateInterval) {
        lastUpdate = millis();
        xOffset--;  // Move o texto para a esquerda
        // Quando o texto sair totalmente, reinicia a posição
        if (xOffset < -textWidth) {
          xOffset = 21;
        }
      }
    } else {
      xOffset = 0;  // Mantém o texto fixo se for menor que 240 pixels
    }

    // Limpa a área do texto
    M5Cardputer.Display.fillRect(0, 33, 240, 15, TFT_BLACK);
    // Desenha o texto na posição atual
    M5Cardputer.Display.drawString(info, xOffset, 33);
    // Linha vermelha decorativa
    M5Cardputer.Display.fillRect(0, 50, 240, 1, TFT_RED);
  }
}

// optional
//void audio_info(const char *info){M5Cardputer.Display.drawString(info, 0, 46);}
//void audio_id3data(const char *info){M5Cardputer.Display.drawString(info, 0, 46);}
//void audio_eof_mp3(const char *info){M5Cardputer.Display.drawString(info, 0, 56);}
//void audio_bitrate(const char *info){M5Cardputer.Display.drawString(info, 0, 70);}
//void audio_commercial(const char *info){M5Cardputer.Display.drawString(info, 0, 50);}
//void audio_showstation(const char *info){M5Cardputer.Display.drawString(info, 0, 50);}
//void audio_icyurl(const char *info){M5Cardputer.Display.drawString(info, 0, 50);}
//void audio_lasthost(const char *info){M5Cardputer.Display.drawString(info, 0, 50);}
//void audio_eof_speech(const char *info){M5Cardputer.Display.drawString(info, 0, 50);}
