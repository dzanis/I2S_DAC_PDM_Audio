#define TEST_DAC

#include <Arduino.h>
#include <SPIFFS.h>
#include "test_dac.wav.h"
#include "test_pdm.wav.h"
#ifdef TEST_DAC
#include "WavDacPlayer.hpp"
#else
#include "WavPdmPlayer.hpp"
#endif


File spiffsFile;
DataFile dac_wav(test_dac_wav, test_dac_wav_len);
DataFile pdm_wav(test_pdm_wav, test_pdm_wav_len);

#ifdef TEST_DAC
WavDacPlayer player;
#else
#if defined(CONFIG_IDF_TARGET_ESP32)
#define OUT_PIN GPIO_NUM_25
#else
#define OUT_PIN GPIO_NUM_6
#endif
WavPdmPlayer player(OUT_PIN);
#endif



void setup() {
    Serial.begin(115200);
    delay(500);
    // Инициализация SPIFFS
    if (!SPIFFS.begin(true)) {
        Serial.println("SPIFFS Mount Failed");
        return;
    }
    
    spiffsFile = SPIFFS.open("/sirena.wav", "r");
    if (!spiffsFile) {
        Serial.println("Failed to open .wav from SPIFFS");
    }

}

void loop()
{
#ifdef TEST_DAC
    player.start(dac_wav, false);// Пример воспроизведения из PROGMEM
#else
    player.start(pdm_wav, false);// Пример воспроизведения из PROGMEM
#endif
    player.start(spiffsFile, false); // Пример воспроизведения из SPIFFS
}
