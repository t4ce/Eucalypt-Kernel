#pragma once
#include <stdint.h>

uint64_t sys_fb_plot_pixel(uint64_t arg0, uint64_t arg1, uint64_t arg2,
                           uint64_t arg3, uint64_t arg4, uint64_t arg5);

uint64_t sys_fb_draw_rect(uint64_t arg0, uint64_t arg1, uint64_t arg2,
                          uint64_t arg3, uint64_t arg4, uint64_t arg5);

uint64_t sys_fb_draw_circle(uint64_t arg0, uint64_t arg1, uint64_t arg2,
                            uint64_t arg3, uint64_t arg4, uint64_t arg5);