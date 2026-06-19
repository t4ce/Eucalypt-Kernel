#include <limine.h>
#include <drivers/framebuffer.h>

extern volatile struct limine_framebuffer_request framebuffer_request;

uint8_t framebuffer_get_info(uint8_t fb, framebuffer_info_t *out) {
    struct limine_framebuffer_response *response = framebuffer_request.response;

    if (!out || !response || fb >= response->framebuffer_count) {
        return 1;
    }

    struct limine_framebuffer *limine_fb = response->framebuffers[fb];
    if (!limine_fb || !limine_fb->address) {
        return 1;
    }

    out->address          = limine_fb->address;
    out->width            = limine_fb->width;
    out->height           = limine_fb->height;
    out->pitch            = limine_fb->pitch;
    out->bpp              = limine_fb->bpp;
    out->red_mask_size    = limine_fb->red_mask_size;
    out->red_mask_shift   = limine_fb->red_mask_shift;
    out->green_mask_size  = limine_fb->green_mask_size;
    out->green_mask_shift = limine_fb->green_mask_shift;
    out->blue_mask_size   = limine_fb->blue_mask_size;
    out->blue_mask_shift  = limine_fb->blue_mask_shift;

    return 0;
}

static uint32_t scale_component(uint8_t value, uint8_t mask_size) {
    if (mask_size == 0) {
        return 0;
    }
    if (mask_size >= 8) {
        return value;
    }

    return value >> (8 - mask_size);
}

uint32_t framebuffer_rgb(uint8_t fb, uint8_t r, uint8_t g, uint8_t b) {
    framebuffer_info_t info;

    if (framebuffer_get_info(fb, &info) != 0) {
        return 0;
    }

    return (scale_component(r, info.red_mask_size)   << info.red_mask_shift)   |
           (scale_component(g, info.green_mask_size) << info.green_mask_shift) |
           (scale_component(b, info.blue_mask_size)  << info.blue_mask_shift);
}

uint8_t framebuffer_put_pixel(uint8_t fb, uint64_t x, uint64_t y, uint32_t color) {
    framebuffer_info_t info;

    if (framebuffer_get_info(fb, &info) != 0 || x >= info.width || y >= info.height) {
        return 1;
    }

    uint8_t *pixel = (uint8_t *)info.address + y * info.pitch + x * (info.bpp / 8);

    switch (info.bpp) {
        case 32:
            *(uint32_t *)pixel = color;
            break;
        case 24:
            pixel[0] = (uint8_t)(color >> 0);
            pixel[1] = (uint8_t)(color >> 8);
            pixel[2] = (uint8_t)(color >> 16);
            break;
        case 16:
            *(uint16_t *)pixel = (uint16_t)color;
            break;
        default:
            return 1;
    }

    return 0;
}

uint8_t plot_pixel(uint8_t fb, uint16_t x, uint16_t y, uint32_t c) {
    return framebuffer_put_pixel(fb, x, y, c);
}

uint8_t draw_rect(uint8_t fb, uint16_t x, uint16_t y, uint16_t w, uint16_t h, bool filled, uint32_t c) {
    framebuffer_info_t info;

    if (framebuffer_get_info(fb, &info) != 0 || x >= info.width || y >= info.height) {
        return 1;
    }

    uint64_t max_x = x + w;
    uint64_t max_y = y + h;

    if (max_x > info.width) {
        max_x = info.width;
    }
    if (max_y > info.height) {
        max_y = info.height;
    }

    if (filled) {
        for (uint64_t py = y; py < max_y; py++) {
            for (uint64_t px = x; px < max_x; px++) {
                framebuffer_put_pixel(fb, px, py, c);
            }
        }
        return 0;
    }

    for (uint64_t px = x; px < max_x; px++) {
        framebuffer_put_pixel(fb, px, y, c);
        if (max_y > (uint64_t)y + 1) {
            framebuffer_put_pixel(fb, px, max_y - 1, c);
        }
    }
    for (uint64_t py = y; py < max_y; py++) {
        framebuffer_put_pixel(fb, x, py, c);
        if (max_x > (uint64_t)x + 1) {
            framebuffer_put_pixel(fb, max_x - 1, py, c);
        }
    }

    return 0;
}

uint8_t draw_circle(uint8_t fb, uint16_t x, uint16_t y, uint16_t r, bool filled, uint32_t c) {
    framebuffer_info_t info;

    if (framebuffer_get_info(fb, &info) != 0 || x >= info.width || y >= info.height) {
        return 1;
    }

    uint32_t radius_sq = r * r;

    if (filled) {
        for (int32_t dy = -(int32_t)r; dy <= (int32_t)r; dy++) {
            for (int32_t dx = -(int32_t)r; dx <= (int32_t)r; dx++) {
                if ((uint32_t)(dx * dx + dy * dy) <= radius_sq) {
                    int64_t px = (int64_t)x + dx;
                    int64_t py = (int64_t)y + dy;
                    if (px >= 0 && py >= 0) {
                        framebuffer_put_pixel(fb, (uint64_t)px, (uint64_t)py, c);
                    }
                }
            }
        }
        return 0;
    }

    uint32_t inner_sq = r > 0 ? (r - 1) * (r - 1) : 0;
    for (int32_t dy = -(int32_t)r; dy <= (int32_t)r; dy++) {
        for (int32_t dx = -(int32_t)r; dx <= (int32_t)r; dx++) {
            uint32_t dist_sq = (uint32_t)(dx * dx + dy * dy);
            if (dist_sq <= radius_sq && dist_sq >= inner_sq) {
                int64_t px = (int64_t)x + dx;
                int64_t py = (int64_t)y + dy;
                if (px >= 0 && py >= 0) {
                    framebuffer_put_pixel(fb, (uint64_t)px, (uint64_t)py, c);
                }
            }
        }
    }
    return 0;
}
