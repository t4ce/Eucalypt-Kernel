#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct framebuffer_info {
    void    *address;
    uint64_t width;
    uint64_t height;
    uint64_t pitch;
    uint16_t bpp;
    uint8_t  red_mask_size;
    uint8_t  red_mask_shift;
    uint8_t  green_mask_size;
    uint8_t  green_mask_shift;
    uint8_t  blue_mask_size;
    uint8_t  blue_mask_shift;
} framebuffer_info_t;

uint8_t  framebuffer_get_info(uint8_t fb, framebuffer_info_t *out);
uint32_t framebuffer_rgb(uint8_t fb, uint8_t r, uint8_t g, uint8_t b);
uint8_t  framebuffer_put_pixel(uint8_t fb, uint64_t x, uint64_t y, uint32_t color);
uint8_t plot_pixel(uint8_t fb, uint16_t x, uint16_t y, uint32_t c);
uint8_t draw_rect(uint8_t fb, uint16_t x, uint16_t y, uint16_t w, uint16_t h, bool filled, uint32_t c);
uint8_t draw_circle(uint8_t fb, uint16_t x, uint16_t y, uint16_t r, bool filled, uint32_t c);
