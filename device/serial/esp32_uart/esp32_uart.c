#ifdef ESP32 

#include "device/serial/serial.h"
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include "Arduino.h"
#include "esp_camera.h"

#define UART_BUF_SIZE 65535

const int RED_LED = 21;
const int WHT_LED = 22;

void serial_init(void)
{
}
void serial_flush(void)
{
}
void serial_capture(Image *img)
{

    int readbyte = 0;
    // Serial.flush();
    // Serial.begin(115200);
    uint32_t startms = millis();
    uint8_t request_start_sequence[4] = "STR";

    uint16_t _blocksize = UART_BUF_SIZE, _lastblocksize = 0;
    uint32_t i = 0, _blockCount = 0, len = 320 * 240;

    if (len < UART_BUF_SIZE)
    {
        _blocksize = len;
    }

    _blockCount = len / _blocksize;
    _lastblocksize = (uint16_t)(len % _blocksize);

    Serial.write(request_start_sequence, 3);
    delay_ms(1);

    uint16_t w = fb->width;
    uint16_t h = fb->height;
    uint16_t f = fb->format;

    Serial.write((uint8_t *)&w, sizeof(w));
    Serial.flush();
    Serial.write((uint8_t *)&h, sizeof(h));
    Serial.flush();
    Serial.write((uint8_t *)&f, sizeof(f));
    Serial.flush();

    // capture chunks of data with the legnth of UART_BUF_SIZE
    for (i = 0; i < _blockCount; i++)
    {
        uint32_t j = 0;
        while (j < _blocksize)
        {
            Serial.readBytes(fb->buf + (i * _blocksize) + j, 1);
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
            Serial.readBytes(fb->buf + (i * _blocksize) + j, 1);
            j++;
            readbyte++;
        }
    }

    if (readbyte == 76800)
    {
        digitalWrite(WHT_LED, HIGH);
        delay_ms(400);
        digitalWrite(WHT_LED, LOW);
        delay_ms(400);
        digitalWrite(WHT_LED, HIGH);
        delay_ms(400);
        digitalWrite(WHT_LED, LOW);
        delay_ms(400);
    }

    delay_ms(200);
}
void serial_send(const Image *img)
{

    // Serial.flush();
    // Serial.begin(2000000);
    uint8_t request_start_sequence[4] = "STW";
    uint8_t a = 0;

    uint16_t _blocksize = UART_BUF_SIZE, _lastblocksize = 0;
    uint32_t i = 0, _blockCount = 0, len = fb->width * fb->height;

    if (fb->len < UART_BUF_SIZE)
        _blocksize = len;

    _blockCount = len / _blocksize;
    _lastblocksize = (uint16_t)(len % _blocksize);

    Serial.write(request_start_sequence, 3);
    delay_ms(1);

    uint16_t w = fb->width;
    uint16_t h = fb->height;
    uint16_t f = fb->format;

    Serial.write((uint8_t *)&w, sizeof(w));
    Serial.flush();
    Serial.write((uint8_t *)&h, sizeof(h));
    Serial.flush();
    Serial.write((uint8_t *)&f, sizeof(f));
    Serial.flush();
    delay_ms(200);

    // send chunks of data with the legnth of UART_BUF_SIZE
    for (i = 0; i < _blockCount; i++)
        Serial.write(fb->buf + (i * _blocksize), _blocksize);
    Serial.flush();

    // send remainder data
    if (_lastblocksize)
        Serial.write(fb->buf + (i * _blocksize), _lastblocksize);

    delay_ms(200);
    // Serial.flush();
    // Serial.begin(115200);
}
void serial_send_jpeg(const Image *img)
{
}
void serial_send_1d(const void *data, uint8_t elem_size, uint32_t length, Serial1DDataType type)
{
}

static void delay_ms(uint32_t ms)
{
    uint32_t startms = millis();
    while (millis() < (startms + ms))
    {
        delay(1);
    }
}

// Define the object
serial_t esp32_uart = {
    .init = serial_init,
    .capture = serial_capture,
    .send = serial_send,
    .sendJPEG = serial_send_jpeg,
    .send1D = serial_send_1d,
    .flush = serial_flush,
};

#endif