#ifndef _WAVPDMPALYER_HPP_
#define _WAVPDMPALYER_HPP_

#include <Arduino.h>
#include <driver/i2s.h>
#include <FS.h>
#include "DataFile.hpp"

#define I2S_SAMPLE_RATE 16000
#define I2S_BUF_SIZE 1024

// Класс для воспроизведения аудио

class WavPdmPlayer
{

public:

    WavPdmPlayer(int out_pin = 25) 
    {
        init(out_pin);
        buffer = new int16_t[I2S_BUF_SIZE];  // Выделяем память для буфера
    }

    ~WavPdmPlayer()
    {
        deinit();
        delete[] buffer;  // Освобождаем память
    }

    void init(int out_pin)
    {
        // Конфигурация I2S
        i2s_config_t i2s_config = {
            .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_PDM),
            .sample_rate = I2S_SAMPLE_RATE,
            .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
            .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
            .communication_format = I2S_COMM_FORMAT_STAND_I2S,
            .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
            .dma_buf_count = 4,
            .dma_buf_len = 1024,
            .use_apll = false,
            .tx_desc_auto_clear = true,
            .fixed_mclk = 0};

        // Установка драйвера I2S
        i2s_driver_install(I2S_NUM_0, &i2s_config, 0, nullptr);

        // Настройка пинов (должна быть только после i2s_driver_install иначе ошибка)
        i2s_pin_config_t i2s_pdm_pins = {
            .bck_io_num = I2S_PIN_NO_CHANGE,
            .ws_io_num = I2S_PIN_NO_CHANGE,
            .data_out_num = out_pin, // пин с которого выходит аудиопоток
            .data_in_num = I2S_PIN_NO_CHANGE};

        i2s_set_pin(I2S_NUM_0, &i2s_pdm_pins);
    }

    void deinit()
    {
        stop();
        i2s_driver_uninstall(I2S_NUM_0); // Деинициализация I2S при разрушении объекта
    }

    template <typename FileType>
    void start(FileType &audioFile, bool async = true)
    {
        if (_task != nullptr) // не запускать проигрывание если оно не закончилось
        {
            return;
        }
        _audioFile = &audioFile; // Сохранение указателя на файл
        audioFile.seek(44); // Пропускаем заголовок WAV
        i2s_zero_dma_buffer(I2S_NUM_0); // Очищаем буферы DMA
        i2s_start(I2S_NUM_0); // Запускаем I2S

        if (async)
        {
            // Запускаем задачу для записи семплов в I2S
            xTaskCreate(i2sWriterTask<FileType>, "i2s Writer Task", 4096, this, 1, &_task);
        }
        else
        {
            i2sWrite<FileType>();
        }
    }

    void stop()
    {
        if (_task != nullptr)
        {
            vTaskDelete(_task);
            _task = nullptr;
        }
        i2s_stop(I2S_NUM_0);
    }

private:
    TaskHandle_t _task = nullptr;
    void *_audioFile = nullptr; // Универсальный указатель на файл
    int16_t * buffer;

    template <typename FileType>
    void i2sWrite()
    {
        FileType *audioFile = static_cast<FileType *>(_audioFile); // Использование сохраненного указателя
        size_t availableBytes = 0;

        while (true)
        {
            size_t bytesRead = audioFile->read((uint8_t *)buffer, I2S_BUF_SIZE * sizeof(int16_t));
            if (bytesRead == 0)
            {
                break;
            }

            for (int i = 0; i < bytesRead / 2; i++)
            {
                buffer[i] = process_sample(buffer[i]);
            }

            size_t bytesWritten;
            i2s_write(I2S_NUM_0, &buffer[0], bytesRead, &bytesWritten, portMAX_DELAY);
        }
    }

    template <typename FileType>
    static void i2sWriterTask(void *param)
    {
        WavPdmPlayer *player = static_cast<WavPdmPlayer *>(param);
        player->i2sWrite<FileType>();
        player->_task = nullptr; // Сброс указателя на задачу
        vTaskDelete(nullptr);                    // Завершаем текущую задачу
        i2s_stop(I2S_NUM_0);
    }

    // Делает сигнал громче, но не превышает предела, который может вызвать искажения при воспроизведении
    int16_t process_sample(int16_t sample)
    {
        float normalised = (float)sample / 32768.0f;
        normalised *= 10.0f;
        if (normalised > 1.0f)
            normalised = 1.0f;
        if (normalised < -1.0f)
            normalised = -1.0f;
        return normalised * 32767.0f;
    }
};
#endif /* _WAVPDMPALYER_HPP_ */