#pragma once
#include <stdint.h>
#include <lib/list.h>

#define INPUT_EVENT_QUEUE_SIZE 256
#define MAX_INPUT_SUBSCRIBERS  256

typedef enum {
    INPUT_EVENT_KEY_PRESS,
    INPUT_EVENT_KEY_RELEASE,
    INPUT_EVENT_MOUSE_MOTION,
    INPUT_EVENT_MOUSE_BUTTON_PRESS,
    INPUT_EVENT_MOUSE_BUTTON_RELEASE,
} input_event_type_t;

typedef enum {
    MOUSE_BUTTON_LEFT   = 1,
    MOUSE_BUTTON_RIGHT  = 2,
    MOUSE_BUTTON_MIDDLE = 4,
} mouse_button_t;

typedef struct {
    input_event_type_t type;
    uint64_t           timestamp;
    union {
        struct {
            uint8_t  scancode;
            uint8_t  keycode;
        } key;
        struct {
            int16_t  dx;
            int16_t  dy;
            uint8_t  buttons;
        } mouse;
    } data;
} input_event_t;

void input_init(void);
void input_subscribe(uint32_t pid);
void input_unsubscribe(uint32_t pid);
void input_event_enqueue(input_event_t *event);
int  input_event_read(uint32_t pid, input_event_t *event);
void ps2_keyboard_init(void);
void ps2_mouse_init(void);
