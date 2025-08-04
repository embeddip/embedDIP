#include <embedDIP_configs.h>

#ifdef DEVICE_ESP32_UART

#include "device/serial/serial.h"
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include "Arduino.h"
#include "esp_camera.h"
#define UART_BUF_SIZE 65535

const int RED_LED = 21;
const int WHT_LED = 22;

static void delay_ms(uint32_t ms)
{
    uint32_t startms = millis();
    while (millis() < (startms + ms))
    {
        delay(1);
    }
}

void serial_init(void)
{
    Serial.flush();
    Serial.setDebugOutput(true);
    Serial.begin(250000);
}
void serial_flush(void)
{
}
void serial_capture(Image *img)
{

    int readbyte = 0;
    Serial.flush();
    Serial.begin(250000);
    uint8_t request_start_sequence[4] = "STR";

    uint16_t _blocksize = UART_BUF_SIZE, _lastblocksize = 0;
    uint32_t i = 0, _blockCount = 0, len = img->height * img->width;

    if (len < UART_BUF_SIZE)
    {
        _blocksize = len;
    }

    _blockCount = len / _blocksize;
    _lastblocksize = (uint16_t)(len % _blocksize);

    Serial.write(request_start_sequence, 3);
    // delay(1);
    // delay_ms(1);

    uint16_t w = img->width;
    uint16_t h = img->height;
    uint16_t f = img->format;
    uint16_t d = img->depth;

    Serial.write((uint8_t *)&w, sizeof(w));
    // Serial.flush();
    Serial.write((uint8_t *)&h, sizeof(h));
    // Serial.flush();
    Serial.write((uint8_t *)&f, sizeof(f));
    // Serial.flush();
    Serial.write((uint8_t *)&d, sizeof(d));

    // capture chunks of data with the legnth of UART_BUF_SIZE
    // char test;
    // Serial.readBytes(&test, 1);
    for (i = 0; i < _blockCount; i++)
    {
        uint32_t j = 0;
        while (j < _blocksize)
        {
            Serial.readBytes(((uint8_t *)img->pixels) + (i * _blocksize) + j, 1);
            j++;
            readbyte++;
        }
    }
    // capture remainder data
    if (_lastblocksize)
    {
        uint32_t j = 0;
        while (j < _lastblocksize)
        {
            Serial.readBytes(((uint8_t *)img->pixels) + (i * _blocksize) + j, 1);
            j++;
            readbyte++;
        }
    }

    if (readbyte == 76800)
    {
        digitalWrite(WHT_LED, HIGH);
        delay(400);
        digitalWrite(WHT_LED, LOW);
        delay(400);
        digitalWrite(WHT_LED, HIGH);
        delay(400);
        digitalWrite(WHT_LED, LOW);
        delay(400);
    }

    delay(200);
}
void serial_send(const Image *img)
{
    // Serial.flush();
    // Serial.begin(2000000);
    uint8_t request_start_sequence[4] = "STW";
    uint16_t _blocksize = UART_BUF_SIZE, _lastblocksize = 0;
    uint32_t i = 0, _blockCount = 0, len = img->width * img->height;

    if (img->size < UART_BUF_SIZE)
        _blocksize = len;

    _blockCount = len / _blocksize;
    _lastblocksize = (uint16_t)(len % _blocksize);

    Serial.write(request_start_sequence, 3);
    // delay(1);

    uint16_t w = img->width;
    uint16_t h = img->height;
    uint16_t f = img->format;
    uint16_t d = img->depth;

    Serial.write((uint8_t *)&w, sizeof(w));
    // Serial.flush();
    Serial.write((uint8_t *)&h, sizeof(h));
    // Serial.flush();
    Serial.write((uint8_t *)&f, sizeof(f));
    // Serial.flush();
    Serial.write((uint8_t *)&d, sizeof(d));

    // send chunks of data with the legnth of UART_BUF_SIZE
    for (i = 0; i < _blockCount; i++)
        Serial.write(((uint8_t *)img->pixels) + (i * _blocksize), _blocksize);
    Serial.flush();

    // send remainder data
    if (_lastblocksize)
        Serial.write(((uint8_t *)img->pixels) + (i * _blocksize), _lastblocksize);
    Serial.flush();

    delay(200);
    // Serial.flush();
    // Serial.begin(115200);
}
void serial_send_jpeg(const Image *img)
{
}
void serial_send_1d(const void *data, uint8_t elem_size, uint32_t length, Serial1DDataType type)
{
}

// Define the object
serial_t esp32_uart = {
    .init = serial_init,
    .capture = serial_capture,
    .send = serial_send,
    .sendJPEG = serial_send_jpeg,
    .send1D = serial_send_1d,
};

#endif