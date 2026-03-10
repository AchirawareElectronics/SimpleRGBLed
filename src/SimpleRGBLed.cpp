/*
 * SimpleRGBLed - Standalone WS2812 RGB LED driver for ESP32.
 * License: MIT
 */

#include "SimpleRGBLed.h"
#include <cstring>

/* ── WS2812 timing (10 MHz RMT resolution = 100ns per tick) ────────── */
#define RMT_RESOLUTION_HZ 10000000
#define T0H_TICKS  3   // 300ns
#define T0L_TICKS  9   // 900ns
#define T1H_TICKS  7   // 700ns
#define T1L_TICKS  6   // 600ns
#define RESET_TICKS 500 // 50µs

/* ── Named colors (English + Spanish) ──────────────────────────────── */

const NamedColor SimpleRGBLed::_colors[] = {
    // English
    {"red",        {255, 0,   0  }},
    {"green",      {0,   255, 0  }},
    {"blue",       {0,   0,   255}},
    {"white",      {255, 255, 255}},
    {"black",      {0,   0,   0  }},
    {"yellow",     {255, 255, 0  }},
    {"cyan",       {0,   255, 255}},
    {"magenta",    {255, 0,   255}},
    {"orange",     {255, 165, 0  }},
    {"pink",       {255, 192, 203}},
    {"purple",     {128, 0,   128}},
    {"brown",      {165, 42,  42 }},
    {"violet",     {128, 0,   128}},
    {"lilac",      {200, 162, 200}},
    {"olive",      {128, 128, 0  }},
    {"lime",       {50,  205, 50 }},
    {"navy",       {0,   0,   128}},
    {"turquoise",  {64,  224, 208}},
    {"salmon",     {250, 128, 114}},
    {"gold",       {255, 215, 0  }},
    {"silver",     {192, 192, 192}},
    {"teal",       {0,   128, 128}},
    {"coral",      {255, 127, 80 }},
    {"crimson",    {220, 20,  60 }},
    {"indigo",     {75,  0,   130}},

    // Spanish
    {"rojo",       {255, 0,   0  }},
    {"verde",      {0,   255, 0  }},
    {"azul",       {0,   0,   255}},
    {"blanco",     {255, 255, 255}},
    {"negro",      {0,   0,   0  }},
    {"amarillo",   {255, 255, 0  }},
    {"cian",       {0,   255, 255}},
    {"magenta",    {255, 0,   255}},
    {"naranja",    {255, 165, 0  }},
    {"rosa",       {255, 192, 203}},
    {"morado",     {128, 0,   128}},
    {"marron",     {165, 42,  42 }},
    {"violeta",    {128, 0,   128}},
    {"lila",       {200, 162, 200}},
    {"verde_oliva",{128, 128, 0  }},
    {"verde_lima", {50,  205, 50 }},
    {"azul_marino",{0,   0,   128}},
    {"turquesa",   {64,  224, 208}},
    {"salmon",     {250, 128, 114}},
    {"dorado",     {255, 215, 0  }},
    {"plata",      {192, 192, 192}},
    {"coral",      {255, 127, 80 }},
    {"carmesi",    {220, 20,  60 }},
};

const uint8_t SimpleRGBLed::_numColors =
    sizeof(SimpleRGBLed::_colors) / sizeof(SimpleRGBLed::_colors[0]);

/* ── Custom RMT encoder for WS2812 byte stream ────────────────────── */

typedef struct {
    rmt_encoder_t base;
    rmt_encoder_t *bytes_enc;
    rmt_encoder_t *copy_enc;
    int state;
    rmt_symbol_word_t reset_sym;
} ws2812_encoder_t;

static size_t _ws2812_encode(rmt_encoder_t *encoder,
                             rmt_channel_handle_t channel,
                             const void *primary_data, size_t data_size,
                             rmt_encode_state_t *ret_state)
{
    ws2812_encoder_t *enc = __containerof(encoder, ws2812_encoder_t, base);
    rmt_encode_state_t session_state = RMT_ENCODING_RESET;
    size_t encoded = 0;

    switch (enc->state) {
    case 0: // encode pixel data
        encoded += enc->bytes_enc->encode(
            enc->bytes_enc, channel, primary_data, data_size, &session_state);
        if (session_state & RMT_ENCODING_COMPLETE) {
            enc->state = 1;
        }
        if (session_state & RMT_ENCODING_MEM_FULL) {
            *ret_state = RMT_ENCODING_MEM_FULL;
            return encoded;
        }
        // fall through
    case 1: // encode reset signal
        encoded += enc->copy_enc->encode(
            enc->copy_enc, channel, &enc->reset_sym,
            sizeof(rmt_symbol_word_t), &session_state);
        if (session_state & RMT_ENCODING_COMPLETE) {
            enc->state = 0;
            *ret_state = RMT_ENCODING_COMPLETE;
        }
        if (session_state & RMT_ENCODING_MEM_FULL) {
            *ret_state = RMT_ENCODING_MEM_FULL;
        }
        return encoded;
    }
    return encoded;
}

static esp_err_t _ws2812_del(rmt_encoder_t *encoder)
{
    ws2812_encoder_t *enc = __containerof(encoder, ws2812_encoder_t, base);
    rmt_del_encoder(enc->bytes_enc);
    rmt_del_encoder(enc->copy_enc);
    free(enc);
    return ESP_OK;
}

static esp_err_t _ws2812_reset(rmt_encoder_t *encoder)
{
    ws2812_encoder_t *enc = __containerof(encoder, ws2812_encoder_t, base);
    rmt_encoder_reset(enc->bytes_enc);
    rmt_encoder_reset(enc->copy_enc);
    enc->state = 0;
    return ESP_OK;
}

bool SimpleRGBLed::_createEncoder(rmt_encoder_handle_t *encoder)
{
    ws2812_encoder_t *enc = (ws2812_encoder_t *)calloc(1, sizeof(ws2812_encoder_t));
    if (!enc) return false;

    enc->base.encode  = _ws2812_encode;
    enc->base.del     = _ws2812_del;
    enc->base.reset   = _ws2812_reset;

    rmt_bytes_encoder_config_t bytes_cfg = {};
    bytes_cfg.bit0.level0    = 1;
    bytes_cfg.bit0.duration0 = T0H_TICKS;
    bytes_cfg.bit0.level1    = 0;
    bytes_cfg.bit0.duration1 = T0L_TICKS;
    bytes_cfg.bit1.level0    = 1;
    bytes_cfg.bit1.duration0 = T1H_TICKS;
    bytes_cfg.bit1.level1    = 0;
    bytes_cfg.bit1.duration1 = T1L_TICKS;
    bytes_cfg.flags.msb_first = 1;

    if (rmt_new_bytes_encoder(&bytes_cfg, &enc->bytes_enc) != ESP_OK) {
        free(enc);
        return false;
    }

    rmt_copy_encoder_config_t copy_cfg = {};
    if (rmt_new_copy_encoder(&copy_cfg, &enc->copy_enc) != ESP_OK) {
        rmt_del_encoder(enc->bytes_enc);
        free(enc);
        return false;
    }

    enc->reset_sym.level0    = 0;
    enc->reset_sym.duration0 = RESET_TICKS;
    enc->reset_sym.level1    = 0;
    enc->reset_sym.duration1 = RESET_TICKS;
    enc->state = 0;

    *encoder = &enc->base;
    return true;
}

/* ── Constructor / Destructor ──────────────────────────────────────── */

SimpleRGBLed::SimpleRGBLed(uint8_t numLeds)
    : _numLeds(numLeds > SIMPLE_RGB_MAX_LEDS ? SIMPLE_RGB_MAX_LEDS : numLeds)
    , _brightness(255)
    , _pin(0)
    , _initialized(false)
    , _rmtChannel(nullptr)
    , _encoder(nullptr)
    , _mutex(nullptr)
{
    memset(_pixelBuf, 0, sizeof(_pixelBuf));
}

SimpleRGBLed::~SimpleRGBLed()
{
    if (_encoder)    rmt_del_encoder(_encoder);
    if (_rmtChannel) rmt_del_channel(_rmtChannel);
    if (_mutex)      vSemaphoreDelete(_mutex);
}

/* ── Initialization ────────────────────────────────────────────────── */

bool SimpleRGBLed::begin(uint8_t pin)
{
    _pin = pin;

    rmt_tx_channel_config_t tx_cfg = {};
    tx_cfg.gpio_num           = (gpio_num_t)pin;
    tx_cfg.clk_src            = RMT_CLK_SRC_DEFAULT;
    tx_cfg.resolution_hz      = RMT_RESOLUTION_HZ;
    tx_cfg.mem_block_symbols  = 64;
    tx_cfg.trans_queue_depth  = 4;

    if (rmt_new_tx_channel(&tx_cfg, &_rmtChannel) != ESP_OK)
        return false;

    if (!_createEncoder(&_encoder)) {
        rmt_del_channel(_rmtChannel);
        _rmtChannel = nullptr;
        return false;
    }

    if (rmt_enable(_rmtChannel) != ESP_OK) {
        rmt_del_encoder(_encoder);
        rmt_del_channel(_rmtChannel);
        _encoder = nullptr;
        _rmtChannel = nullptr;
        return false;
    }

    _initialized = true;
    offAll();
    return true;
}

/* ── Brightness helpers ────────────────────────────────────────────── */

void SimpleRGBLed::_applyBrightness(uint8_t r, uint8_t g, uint8_t b, uint8_t *out)
{
    // GRB order for WS2812
    out[0] = (uint8_t)((g * _brightness) >> 8);
    out[1] = (uint8_t)((r * _brightness) >> 8);
    out[2] = (uint8_t)((b * _brightness) >> 8);
}

bool SimpleRGBLed::_transmit()
{
    if (!_initialized) return false;

    rmt_transmit_config_t tx_cfg = {};
    tx_cfg.loop_count = 0;

    esp_err_t err = rmt_transmit(
        _rmtChannel, _encoder, _pixelBuf, _numLeds * 3, &tx_cfg);
    if (err != ESP_OK) return false;

    rmt_tx_wait_all_done(_rmtChannel, portMAX_DELAY);
    return true;
}

/* ── Mutex ─────────────────────────────────────────────────────────── */

void SimpleRGBLed::_lock()
{
    if (_mutex) xSemaphoreTake(_mutex, portMAX_DELAY);
}

void SimpleRGBLed::_unlock()
{
    if (_mutex) xSemaphoreGive(_mutex);
}

void SimpleRGBLed::enableMutex()
{
    if (!_mutex) _mutex = xSemaphoreCreateMutex();
}

/* ── Color lookup ──────────────────────────────────────────────────── */

const NamedColor *SimpleRGBLed::findColor(const char *name)
{
    for (uint8_t i = 0; i < _numColors; i++) {
        if (strcasecmp(name, _colors[i].name) == 0)
            return &_colors[i];
    }
    return nullptr;
}

uint8_t SimpleRGBLed::colorCount() { return _numColors; }

const NamedColor *SimpleRGBLed::colorAt(uint8_t idx)
{
    return (idx < _numColors) ? &_colors[idx] : nullptr;
}

/* ── Public API ────────────────────────────────────────────────────── */

void SimpleRGBLed::setColor(const char *colorName, uint8_t index)
{
    const NamedColor *c = findColor(colorName);
    if (c)
        setRGB(c->color.r, c->color.g, c->color.b, index);
    else
        off(index);
}

void SimpleRGBLed::setRGB(uint8_t r, uint8_t g, uint8_t b, uint8_t index)
{
    if (index >= _numLeds) return;
    _lock();
    _applyBrightness(r, g, b, &_pixelBuf[index * 3]);
    _transmit();
    _unlock();
}

void SimpleRGBLed::setHex(uint32_t hexColor, uint8_t index)
{
    setRGB((hexColor >> 16) & 0xFF,
           (hexColor >> 8)  & 0xFF,
            hexColor        & 0xFF, index);
}

void SimpleRGBLed::setAll(const char *colorName)
{
    const NamedColor *c = findColor(colorName);
    if (c)
        setAllRGB(c->color.r, c->color.g, c->color.b);
    else
        offAll();
}

void SimpleRGBLed::setAllRGB(uint8_t r, uint8_t g, uint8_t b)
{
    _lock();
    for (uint8_t i = 0; i < _numLeds; i++)
        _applyBrightness(r, g, b, &_pixelBuf[i * 3]);
    _transmit();
    _unlock();
}

void SimpleRGBLed::setAllHex(uint32_t hexColor)
{
    setAllRGB((hexColor >> 16) & 0xFF,
              (hexColor >> 8)  & 0xFF,
               hexColor        & 0xFF);
}

void SimpleRGBLed::off(uint8_t index)
{
    setRGB(0, 0, 0, index);
}

void SimpleRGBLed::offAll()
{
    setAllRGB(0, 0, 0);
}

void SimpleRGBLed::setBrightness(uint8_t brightness)
{
    _brightness = brightness ? brightness : 1;
}

uint8_t SimpleRGBLed::getBrightness() const { return _brightness; }

void SimpleRGBLed::blink(const char *colorName, uint8_t times,
                         uint16_t onMs, uint16_t offMs, uint8_t index)
{
    for (uint8_t i = 0; i < times; i++) {
        setColor(colorName, index);
        delay(onMs);
        off(index);
        if (i < times - 1) delay(offMs);
    }
}

RGBColor SimpleRGBLed::getPixel(uint8_t index) const
{
    RGBColor c = {0, 0, 0};
    if (index < _numLeds) {
        // buffer is in GRB order
        c.g = _pixelBuf[index * 3];
        c.r = _pixelBuf[index * 3 + 1];
        c.b = _pixelBuf[index * 3 + 2];
    }
    return c;
}

uint8_t SimpleRGBLed::numLeds() const { return _numLeds; }
