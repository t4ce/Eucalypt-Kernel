#include <stdint.h>
#include <limine.h>
#include <drivers/framebuffer.h>

extern volatile struct limine_framebuffer_request framebuffer_request;

uint8_t plot_pixel(uint8_t fb, uint16_t x, uint16_t y, uint32_t c) {
    if (fb >= framebuffer_request.response->framebuffer_count 
        || x >= framebuffer_request.response->framebuffers[fb]->width 
        || y >= framebuffer_request.response->framebuffers[fb]->height) {

        return 1;
    }

    uint32_t *p = (uint32_t *)framebuffer_request.response->framebuffers[fb]->address;
    p[y * framebuffer_request.response->framebuffers[fb]->width + x] = c;
    
    return 0;
}

uint8_t draw_rect(uint8_t fb, uint16_t x, uint16_t y, uint16_t w, uint16_t h, bool filled, uint32_t c) {
    if (fb >= framebuffer_request.response->framebuffer_count 
        || x >= framebuffer_request.response->framebuffers[fb]->width 
        || y >= framebuffer_request.response->framebuffers[fb]->height) {

        return 1;
    }

    uint32_t *p = (uint32_t *)framebuffer_request.response->framebuffers[fb]->address;
    
    if (filled) {
        for (uint16_t i = 1; i < w - 1; i++) {
            for (uint16_t j = 1; j < h - 1; j++) {
                p[(y + i) * framebuffer_request.response->framebuffers[fb]->width + x + j] = c;
            }
        }
    }

    for (uint16_t i = 0; i < w; i++) {
        for (uint16_t j = 0; j < h; j++) {
            p[(y + j) * framebuffer_request.response->framebuffers[fb]->width + x + i] = c;
        }
    }

    return 0;
}

uint8_t draw_circle(uint8_t fb, uint16_t x, uint16_t y, uint16_t r, bool filled, uint32_t c) {
    if (fb >= framebuffer_request.response->framebuffer_count 
        || x >= framebuffer_request.response->framebuffers[fb]->width 
        || y >= framebuffer_request.response->framebuffers[fb]->height) {

        return 1;
    }

    uint32_t *p = (uint32_t *)framebuffer_request.response->framebuffers[fb]->address;
    
    if (filled) {
        for (uint16_t i = 0; i < r; i++) {
            for (uint16_t j = 0; j < r; j++) {
                if (i * i + j * j <= r * r) {
                    p[(y + i) * framebuffer_request.response->framebuffers[fb]->width + x + j] = c;
                }
            }
        }
    }

    for (uint16_t i = 0; i < r; i++) {
        for (uint16_t j = 0; j < r; j++) {
            if (i * i + j * j <= r * r) {
                p[(y + i) * framebuffer_request.response->framebuffers[fb]->width + x + j] = c;
            }
        }
    }
    return 0;
}
