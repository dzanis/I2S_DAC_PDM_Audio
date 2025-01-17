# I2S_DAC_PDM_Audio

`I2S_DAC_PDM_Audio` — это проект на основе ESP32 для воспроизведения аудио с использованием встроенного DAC и интерфейса I2S. Проект поддерживает работу с аудио в формате WAV и воспроизводит его через встроенный DAC или выводит в PDM формат на пине GPIO25.

## Формат WAV

Проект работает с монофоническими WAV файлами, имеющими следующие характеристики:
- **Частота дискретизации**: 16000 Гц
- **Глубина звука**: 16 бит
- **Каналы**: Моно

WAV файлы, которые не соответствуют этим параметрам, могут воспроизводиться некорректно или не воспроизводиться вовсе.

## Основные возможности

- **WAV воспроизведение через DAC**: Используйте класс `WavDacPlayer.hpp` для воспроизведения через встроенный DAC ESP32 только на пине GPIO25.
- **WAV воспроизведение через PDM**: Используйте класс `WavPdmPlayer.hpp` для вывода аудио в формате PDM (Pulse Density Modulation) на тот же пин GPIO25 или любой другой.
- **Аудио файлы из SPIFFS**: WAV файлы загружаются в файловую систему ESP32 (SPIFFS) из папки `data`, используя команду из PlatformIO.
- **Воспроизведение из PROGMEM**: Класс `DataFile` позволяет проигрывать аудиофайлы, хранящиеся в памяти микроконтроллера (PROGMEM), что полезно для воспроизведения предопределенных звуков без необходимости использования внешних файловых систем.

## Подключение

- **DAC и PDM выводы**: Аудиосигнал выводится через встроенный DAC ESP32 или в формате PDM на пин GPIO25. Подключите этот пин к усилителю или динамику для воспроизведения звука.

## Загрузка аудиофайлов

WAV файлы загружаются в файловую систему SPIFFS, используя платформу PlatformIO. Для этого разместите файлы в папке `data`. Загружать их в файловую систему можно через интерфейс PlatformIO:

- В интерфейсе PlatformIO откройте вкладку "Project Tasks" (Задачи проекта).
- Найдите раздел `Platform` и выберите "Upload Filesystem Image" (Загрузка образа файловой системы).
- Нажмите кнопку запуска задачи, и файлы из папки `data` будут загружены в SPIFFS ESP32.

Этот способ может быть более удобным для тех, кто предпочитает работать через графический интерфейс.

Или выполните команду:

```sh
pio run --target uploadfs
```
## Пример использования DAC и SPIFFS

```cpp
#include <Arduino.h>
#include <SPIFFS.h>
#include "WavDacPlayer.hpp"

// Создаем экземпляр класса WavDacPlayer
WavDacPlayer player;
File audioFile;

void setup() {
    Serial.begin(115200);

    // Инициализация SPIFFS
    if (!SPIFFS.begin()) {
        Serial.println("SPIFFS Mount Failed");
        return;
    }

    // Открытие файла из SPIFFS
    audioFile = SPIFFS.open("/sample.wav");
    if (!audioFile) {
        Serial.println("Failed to open audio file.");
        return;
    }

    // Начало воспроизведения аудио файла асинхронно
    player.start(audioFile);
}

void loop() {
    // Основной код программы, который выполняется во время воспроизведения аудио
    // Например, мигать светодиодом
    digitalWrite(LED_BUILTIN, HIGH);
    delay(500);
    digitalWrite(LED_BUILTIN, LOW);
    delay(500);
}
```

## Воспроизведение аудио из PROGMEM

Для проигрывания аудиофайлов из PROGMEM используется класс `DataFile`.
Файлы в PROGMEM удобны тем что они находятся в прошивке и одновременно заливаются при обновлении через веб интерфейс (OTA).

#### Конвертация WAV в PROGMEM

Лично я конвертировал wav в массив hex своим расширением к PlatformIO,которое я пока что не распространяю (потому что не законченное).
Но можно воспользоваться таким вариантом по ссылке [wav2asm_c](https://sampawno.ru/viewtopic.php?f=116&t=13202)

## Пример использования DAC и DataFile

```cpp
#include <Arduino.h>
#include <SPIFFS.h>
#include "WavDacPlayer.hpp"
#include "test_dac.wav.h"

// Создаем экземпляр класса WavDacPlayer
WavDacPlayer player;
DataFile dac_wav(test_dac_wav, test_dac_wav_len);

void setup() {
    player.start(dac_wav);
}

void loop() {
}
```
#### Примечания
Если проигрывать синхронно , то программа остановится пока проигрывание не закончится.

```cpp
player.start(dac_wav,false); // здесь программа останавливается пока не закончится проигрывание
```


## Преимущества PDM

Использование PDM (Pulse Density Modulation) имеет ряд преимуществ по сравнению с встроенным DAC:

- **Широкая совместимость**: PDM воспроизведение поддерживается на большем числе моделей ESP32, в то время как встроенный DAC доступен не на всех версиях.
- **Низкий уровень шума**: В отличие от встроенного DAC, который может вносить заметный шум в аудиосигнал, PDM технология обеспечивает более чистый звук благодаря высокочастотному преобразованию сигнала.

### Совместимость ESP32

Ниже представлена таблица, отображающая поддержку встроенного DAC и PDM на различных моделях ESP32:

| Модель ESP32           | Поддержка DAC | Поддержка PDM |
|------------------------|---------------|---------------|
| ESP32 (WROOM, WROVER)  | Да            | Да            |
| ESP32-S2               | Нет           | Да            |
| ESP32-S3               | Нет           | Да            |
| ESP32-C3               | Нет           | Да            |
| ESP32-H2               | Нет           | Да            |
| ESP32-C6               | Нет           | Да            |

#### Примечания

- **ESP32 (WROOM, WROVER)**: Эти модели поддерживают как встроенный DAC, так и PDM. Они наиболее универсальны для аудиопроектов, требующих аналогового и цифрового вывода звука.
- **ESP32-S2, S3, C3, H2, C6**: Эти модели не имеют встроенного DAC, но могут выводить звук через PDM, что делает их подходящими для приложений с цифровым аудио.

Таким образом, если ваш проект требует использования аудио на устройствах ESP32, выбор PDM может быть более предпочтительным, особенно на устройствах без встроенного DAC.

## Пример использования PDM и DataFile

```cpp
#include <Arduino.h>
#include "WavPdmPlayer.hpp"
#include "test_pdm.wav.h"

// Создаем экземпляр класса WavPdmPlayer
WavPdmPlayer player;
DataFile pdm_wav(test_pdm_wav, test_pdm_wav_len);

void setup() {
    player.start(pdm_wav);
}

void loop() {
}
```
#### Примечания
А вот для ESP32 C3 нужно переопределить пин, так как в нём нету 25 пина.

```cpp

WavPdmPlayer player(GPIO_NUM_6); // Создаем экземпляр класса WavPdmPlayer с выходом на пин 6

```

## Лицензия

Этот проект распространяется под лицензией MIT. Вы можете свободно использовать и модифицировать его в своих проектах.

