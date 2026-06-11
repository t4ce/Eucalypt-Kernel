#include <stdint.h>
#include <drivers/framebuffer.h>
#include <syscalls/syscall_framebuffer.h>

uint64_t sys_fb_plot_pixel(uint64_t arg0, uint64_t arg1, uint64_t arg2,
                           uint64_t arg3, uint64_t arg4, uint64_t arg5) {
    (void)arg4; (void)arg5;
    uint8_t  fb = (uint8_t)arg0;
    uint16_t x  = (uint16_t)arg1;
    uint16_t y  = (uint16_t)arg2;
    uint32_t c  = (uint32_t)arg3;
    return (uint64_t)plot_pixel(fb, x, y, c);
}

uint64_t sys_fb_draw_rect(uint64_t arg0, uint64_t arg1, uint64_t arg2,
                          uint64_t arg3, uint64_t arg4, uint64_t arg5) {
    uint8_t  fb     = (uint8_t)arg0;
    uint16_t x      = (uint16_t)arg1;
    uint16_t y      = (uint16_t)arg2;
    uint16_t w      = (uint16_t)arg3;
    uint16_t h      = (uint16_t)arg4;
    bool     filled = (bool)(arg5 & 1);
    uint32_t c      = (uint32_t)(arg5 >> 32);
    return (uint64_t)draw_rect(fb, x, y, w, h, filled, c);
}

uint64_t sys_fb_draw_circle(uint64_t arg0, uint64_t arg1, uint64_t arg2,
                            uint64_t arg3, uint64_t arg4, uint64_t arg5) {
    (void)arg5;
    uint8_t  fb     = (uint8_t)arg0;
    uint16_t x      = (uint16_t)arg1;
    uint16_t y      = (uint16_t)arg2;
    uint16_t r      = (uint16_t)arg3;
    bool     filled = (bool)(arg4 & 1);
    uint32_t c      = (uint32_t)(arg4 >> 32);
    return (uint64_t)draw_circle(fb, x, y, r, filled, c);
}