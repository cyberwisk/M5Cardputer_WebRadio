
# M5Cardputer_WebRadio

M5Cardputer_WebRadio baseado nas bibliotecas: 

M5Unified : https://github.com/m5stack/M5Unified 

A lista de rádios fica em um arquivo texto(station_list.txt) na raiz do cartão SD
Ex:
Nome da Radio01,http://Link da Radio/stream01
Nome da Radio02,http://Link da Radio/stream02
- Tecla R reseta a conexão com o servidor se a radio travar ou não iniciar
- Tecla M liga e desliga o mudo
- Tecla F liga e desliga o FFT
- Capacidade de executar rádios AAC ou MP3
- Salva configurações de WIFI na memória

- Arquivo station_list.txt de exemplo
https://github.com/cyberwisk/M5Cardputer_WebRadio/blob/main/M5Cardputer_WebRadio/station_list.txt

Baseado nas bibliotecas:
M5Unified : https://github.com/m5stack/M5Unified
ESP32-audioI2S Versão 3.0.13 : https://github.com/schreibfaul1/ESP32-audioI2S

Aurelio


![image](https://github.com/cyberwisk/M5Cardputer_WebRadio/assets/3136312/9bd48f53-334c-43c1-8226-1ece040430c5)
----
Wifi:

Com ajuda da biblioteca [Preferences.h](https://github.com/espressif/arduino-esp32/tree/master/libraries/Preferences) agora é possivel salvar as configurações de Wifi na 

EEPROM do StamoS3

Assim que ligar o dispositivo, ele vai pedir as configurações de SSID e Senha do WIFI e salvar 

![image](https://github.com/cyberwisk/M5Cardputer_WebRadio/assets/3136312/531dfc77-a9b6-4a27-82ec-f0d6eeed2621)

---------------
* Configurações necessárias do StampS3 na IDE do arduino:

<img width="858" height="912" alt="image" src="https://github.com/user-attachments/assets/e14d7af5-be04-4ffd-b0f8-c96559b7589f" />
