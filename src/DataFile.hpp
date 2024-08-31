#ifndef _DATAFILE_HPP_
#define _DATAFILE_HPP_

#include <Arduino.h>

// Класс DataFile для работы с данными из PROGMEM
class DataFile
{
public:
    DataFile(const uint8_t *data, size_t length) : data(data), length(length), position(0) {}

    operator bool() const
    {
        return data != nullptr && length != 0;
    }

    size_t read(uint8_t *buffer, size_t len)
    {
        if (position >= length)
            return 0;
        size_t bytesToRead = min(len, length - position);
        memcpy_P(buffer, data + position, bytesToRead);
        position += bytesToRead;
        return bytesToRead;
    }

    size_t available()
    {
        return length - position;
    }

    void seek(size_t pos)
    {
        if (pos < length)
        {
            position = pos;
        }
    }

    void close()
    {
        data = nullptr;
        length = 0;
    }

private:
    const uint8_t *data;
    size_t length;
    size_t position;
};

#endif /* _DATAFILE_HPP_ */