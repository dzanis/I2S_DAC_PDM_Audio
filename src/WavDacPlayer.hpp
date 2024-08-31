#ifndef _WAVDACPALYER_HPP_
#define _WAVDACPALYER_HPP_

#include <Arduino.h>
#include <driver/i2s.h>
#include <FS.h>
#include "DataFile.hpp"

#define I2S_SAMPLE_RATE 16000
#define I2S_BUF_SIZE 1024

// Класс для воспроизведения аудио

class WavDacPlayer
{

public:
    WavDacPlayer()
    {
        init();
        buffer = new int16_t[I2S_BUF_SIZE];  // Выделяем память для буфера
    }

    ~WavDacPlayer()
    {
        deinit();
        delete[] buffer;  // Освобождаем память
    }

    void init()
    {
        // Конфигурация I2S
        i2s_config_t i2s_config = {
            .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_DAC_BUILT_IN),
            .sample_rate = I2S_SAMPLE_RATE,
            .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
            .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
            .communication_format = I2S_COMM_FORMAT_STAND_MSB,
            .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
            .dma_buf_count = 4,
            .dma_buf_len = I2S_BUF_SIZE,
            .use_apll = true,
            .tx_desc_auto_clear = true,
            .fixed_mclk = 0};

        // Установка драйвера I2S
        i2s_driver_install(I2S_NUM_0, &i2s_config, 0, nullptr);
        i2s_set_dac_mode(I2S_DAC_CHANNEL_BOTH_EN); // Используем оба канала DAC а иначе звука нет
    }

    void deinit()
    {
        stop();
        i2s_driver_uninstall(I2S_NUM_0); // Деинициализация I2S при разрушении объекта
    }

    /**
     * @brief Начинает проигрывание аудиофайла через аудиосистему I2S.
     *
     * Этот метод используется для запуска воспроизведения аудиофайла. Он может работать в двух режимах:
     * - **Асинхронный режим** (по умолчанию): Аудио проигрывается в фоновом режиме, и основной код продолжает выполняться.
     * - **Синхронный режим**: Аудио проигрывается в том же потоке, и выполнение основного кода приостанавливается до окончания воспроизведения.
     *
     * @tparam FileType Тип объекта аудиофайла, который будет воспроизводиться.
     * @param audioFile Ссылка на аудиофайл, который будет воспроизводиться.
     * @param async Если true (по умолчанию), аудио будет проигрываться асинхронно в фоновом режиме.
     *              Если false, воспроизведение будет выполняться синхронно, блокируя выполнение основного кода.
     *
     * @details
     * 1. Сначала метод проверяет, не идет ли уже воспроизведение. Если оно идет, новое не запускается.
     * 2. Метод запоминает переданный аудиофайл и пропускает первые 44 байта (это заголовок WAV-файла, который не нужно воспроизводить).
     * 3. Затем метод подготавливает и запускает систему I2S для воспроизведения аудио.
     * 4. Если выбран асинхронный режим (по умолчанию), создается фоновая задача для воспроизведения аудио.
     * 5. Если выбран синхронный режим, воспроизведение начинается немедленно, и код ждет его завершения.
     */
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
            // Преобразование в unsigned 16-bit , проще говоря убераем отрицательные числа
            for (int i = 0; i < bytesRead / 2; i++)
            {
                buffer[i] += process_sample(buffer[i]);

            }

            size_t bytesWritten;
            i2s_write(I2S_NUM_0, &buffer[0], bytesRead, &bytesWritten, portMAX_DELAY);
        }
    }

    template <typename FileType>
    static void i2sWriterTask(void *param)
    {

        WavDacPlayer *player = static_cast<WavDacPlayer *>(param);
        player->i2sWrite<FileType>();
        player->_task = nullptr; // Сброс указателя на задачу
        vTaskDelete(nullptr);                    // Завершаем текущую задачу
        i2s_stop(I2S_NUM_0);
    }

    // Преобразование в unsigned 16-bit , проще говоря убераем отрицательные числа
    int16_t process_sample(int16_t sample)
    {
        return sample + 32768;
    }
};
#endif /* _WAVDACPALYER_HPP_ */