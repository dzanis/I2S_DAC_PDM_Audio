/** 
 * Сделано по примеру https://github.com/atomic14/esp32_audio/blob/master/dac_i2s_output/src/DACOutput.cpp
 * Реализация довольно сложная с использованием "event queue" и в чём преймущество такого подхода
 * для меня секрет.В последствии по качеству звучания разницы не замечено.
*/
#ifndef _WAWDACPLAYER_Q_HPP_
#define _WAWDACPLAYER_Q_HPP_

#include <Arduino.h>
#include <driver/i2s.h>
#include <FS.h>

#define I2S_SAMPLE_RATE 16000
#define NUM_FRAMES_TO_SEND 1024

// Класс для воспроизведения аудио
class WavDacPlayer {



// I2S write task
TaskHandle_t m_i2sWriterTaskHandle;
// i2s writer queue
QueueHandle_t m_i2sQueue;

Stream * m_audioStream;

public:
    WavDacPlayer()
    {
        ////output = new DACOutput();  // Создаем DACOutput по умолчанию
        // Конфигурация I2S
        i2s_config_t i2s_config = {
            .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_DAC_BUILT_IN),
            .sample_rate = I2S_SAMPLE_RATE,
            .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
            .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
            .communication_format = I2S_COMM_FORMAT_STAND_MSB,
            .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
            .dma_buf_count = 4,
            .dma_buf_len = 1024,
            .use_apll = true,
            .tx_desc_auto_clear = true, // автоматически очищать FIFO
            .fixed_mclk = 0};

        // Установка драйвера I2S
        i2s_driver_install(I2S_NUM_0, &i2s_config, 4, &m_i2sQueue);
        i2s_set_dac_mode(I2S_DAC_CHANNEL_BOTH_EN); // Используем оба канала DAC а иначе звука нет
    }

    ~WavDacPlayer()
    {
        stop();
        i2s_driver_uninstall(I2S_NUM_0); // Деинициализация I2S при разрушении объекта
    }

    // Преобразование в unsigned 16-bit , проще говоря убераем отрицательные числа
    void process_sample(uint16_t *sample, size_t size)
    {
        for (size_t i = 0; i < size; i++)
        {
            sample[i] += 32768;
        }
    }

    // Метод для чтения семплов из аудиопотока
    size_t readSamples(uint8_t *buf, size_t size)
    {
        size_t availableBytes = 0;
        while (availableBytes < size && m_audioStream->available())
        {
            buf[availableBytes++] = m_audioStream->read();
        }

        process_sample((uint16_t *)buf, availableBytes/2);

        return availableBytes;
    }


 // Метод, который будет запускаться в виде задачи FreeRTOS
static void i2sWriterTask(void *param)
{
    //WavDacPlayer  *player = (WavDacPlayer  *)param;
    WavDacPlayer *player = static_cast<WavDacPlayer*>(param);
    int availableBytes = 0;
    int buffer_position = 0;
    uint16_t buffer[NUM_FRAMES_TO_SEND];
    while (true)
    {
        // wait for some data to be requested
        i2s_event_t evt;
        if (xQueueReceive(player->m_i2sQueue, &evt, portMAX_DELAY) == pdPASS)
        {
            if (evt.type == I2S_EVENT_TX_DONE)
            {
                size_t bytesWritten = 0;
                do
                {
                    if (availableBytes == 0)
                        {
                            availableBytes = player->readSamples((uint8_t *)buffer, sizeof(buffer));
                            buffer_position = 0;

                            if (availableBytes == 0)
                            {
                                Serial.println("Finished Playing Audio");
                                player->stop(); // Правильное завершение задачи
                                return;
                            }
                        }

                    // do we have something to write?
                    if (availableBytes > 0)
                    {
                        // write data to the i2s peripheral
                        i2s_write(I2S_NUM_0, buffer_position + (uint8_t *)buffer,
                                  availableBytes, &bytesWritten, portMAX_DELAY);
                        availableBytes -= bytesWritten;
                        buffer_position += bytesWritten;
                    }
                } while (bytesWritten > 0);
            }
        }
    }
}

// Метод для воспроизведения из любого источника, реализующего интерфейс File-like

// Метод для запуска задачи воспроизведения
    void start(Stream *audioStream) {

        m_audioStream = audioStream;
        if (!audioStream) {
            Serial.println("Failed to open file for reading");
            return;
        }

        // Пропуск заголовка WAV
        //audioStream->seek(44);

        // Очищаем буферы DMA
        i2s_zero_dma_buffer(I2S_NUM_0);

        // Запускаем I2S 
        i2s_start(I2S_NUM_0);

        // Запускаем задачу для записи семплов в I2S, передавая файл
        xTaskCreate(i2sWriterTask, "i2s Writer Task", 4096, this, 1, &m_i2sWriterTaskHandle);
    }

    // Метод для остановки воспроизведения
    void stop()
    {
        if (m_i2sWriterTaskHandle != NULL)
        {
            vTaskDelete(m_i2sWriterTaskHandle);
            m_i2sWriterTaskHandle = NULL;
        }

        i2s_stop(I2S_NUM_0);

        // Не закрываем m_audioStream, так как он может понадобиться для повторного воспроизведения
    }

};

// Создание экземпляра плеера
//extern WavDacPlayer WAVDACPLAYER;

#endif /* _WAWDACPLAYER_Q_HPP_ */