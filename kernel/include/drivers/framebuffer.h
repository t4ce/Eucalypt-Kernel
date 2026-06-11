#pragma once

#include <stdint.h>

uint8_t plot_pixel(uint8_t fb, uint16_t x, uint16_t y, uint32_t c);
uint8_t draw_rect(uint8_t fb, uint16_t x, uint16_t y, uint16_t w, uint16_t h, bool filled, uint32_t c);
uint8_t draw_circle(uint8_t fb, uint16_t x, uint16_t y, uint16_t r, bool filled, uint32_t c);