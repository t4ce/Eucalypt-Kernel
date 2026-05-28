#include <stdint.h>
#include <stddef.h>
#include <mem.h>
#include <mm/heap.h>
#include <drivers/fs/devfs/devfs.h>
#include <drivers/tty.h>

static tty_t   ttys[TTY_MAX];
static uint8_t active_tty = 0;
static void  (*global_output_fn)(tty_t *tty, char c) = NULL;

static void ring_push(tty_ring_t *r, uint8_t c) {
    if (r->count >= TTY_BUF_SIZE) return;
    r->data[r->tail] = c;
    r->tail = (r->tail + 1) % TTY_BUF_SIZE;
    r->count++;
}

static int ring_pop(tty_ring_t *r, uint8_t *out) {
    if (r->count == 0) return 0;
    *out = r->data[r->head];
    r->head = (r->head + 1) % TTY_BUF_SIZE;
    r->count--;
    return 1;
}

static void tty_putchar_raw(tty_t *tty, char c) {
    if ((tty->termios.c_oflag & OPOST) && (tty->termios.c_oflag & ONLCR) && c == '\n')
        tty->putchar(tty, '\r');
    tty->putchar(tty, c);
}

void tty_input(tty_t *tty, char c) {
    if (tty->termios.c_iflag & ICRNL && c == '\r')
        c = '\n';

    if (tty->termios.c_lflag & ICANON) {
        if (c == tty->termios.c_cc[VERASE]) {
            if (tty->raw.count > 0) {
                tty->raw.tail = (tty->raw.tail == 0 ? TTY_BUF_SIZE : tty->raw.tail) - 1;
                tty->raw.count--;
                if (tty->termios.c_lflag & ECHOE) {
                    tty->putchar(tty, '\b');
                    tty->putchar(tty, ' ');
                    tty->putchar(tty, '\b');
                }
            }
            return;
        }

        if (tty->termios.c_lflag & ECHO)
            tty_putchar_raw(tty, c);

        ring_push(&tty->raw, (uint8_t)c);

        if (c == '\n' || c == tty->termios.c_cc[VEOF]) {
            uint8_t byte;
            while (ring_pop(&tty->raw, &byte))
                ring_push(&tty->cooked, byte);
        }
    } else {
        if (tty->termios.c_lflag & ECHO)
            tty_putchar_raw(tty, c);
        ring_push(&tty->cooked, (uint8_t)c);
    }
}

int32_t tty_write(tty_t *tty, const uint8_t *buf, uint32_t count) {
    if (!tty || !tty->putchar) return -1;
    for (uint32_t i = 0; i < count; i++)
        tty_putchar_raw(tty, (char)buf[i]);
    return (int32_t)count;
}

int32_t tty_read(tty_t *tty, uint8_t *buf, uint32_t count) {
    if (!tty || !buf || count == 0) return -1;
    uint32_t n = 0;
    while (n < count) {
        uint8_t c;
        if (!ring_pop(&tty->cooked, &c)) break;
        buf[n++] = c;
        if (tty->termios.c_lflag & ICANON && c == '\n') break;
    }
    return (int32_t)n;
}

tty_t *tty_get_active(void) {
    return &ttys[active_tty];
}

tty_t *tty_get(uint8_t index) {
    if (index >= TTY_MAX) return NULL;
    return &ttys[index];
}

void tty_switch(uint8_t index) {
    if (index >= TTY_MAX) return;
    ttys[active_tty].active = 0;
    active_tty = index;
    ttys[active_tty].active = 1;
}

static int32_t devfs_tty_read(devfs_dev_t *dev, void *buf, uint32_t count) {
    tty_t *tty = (tty_t *)dev->priv;
    return tty_read(tty, (uint8_t *)buf, count);
}

static int32_t devfs_tty_write(devfs_dev_t *dev, const void *buf, uint32_t count) {
    tty_t *tty = (tty_t *)dev->priv;
    return tty_write(tty, (const uint8_t *)buf, count);
}

static int32_t devfs_tty_alias_read(devfs_dev_t *dev, void *buf, uint32_t count) {
    (void)dev;
    return tty_read(tty_get_active(), (uint8_t *)buf, count);
}

static int32_t devfs_tty_alias_write(devfs_dev_t *dev, const void *buf, uint32_t count) {
    (void)dev;
    return tty_write(tty_get_active(), (const uint8_t *)buf, count);
}

void tty_init(void (*output_fn)(tty_t *tty, char c)) {
    global_output_fn = output_fn;

    for (uint8_t i = 0; i < TTY_MAX; i++) {
        tty_t *t = &ttys[i];
        memset(t, 0, sizeof(tty_t));

        t->index  = i;
        t->active = (i == 0);

        t->termios.c_iflag = ICRNL;
        t->termios.c_oflag = OPOST | ONLCR;
        t->termios.c_lflag = ECHO | ECHOE | ICANON | ISIG;
        t->termios.c_cc[VINTR]  = 0x03;
        t->termios.c_cc[VERASE] = 0x7F;
        t->termios.c_cc[VKILL]  = 0x15;
        t->termios.c_cc[VEOF]   = 0x04;
        t->termios.c_cc[VMIN]   = 1;

        t->putchar = output_fn;

        char name[8];
        name[0] = 't'; name[1] = 't'; name[2] = 'y';
        name[3] = '0' + i; name[4] = '\0';

        devfs_register(name, devfs_tty_read, devfs_tty_write, t);
    }

    devfs_register("tty", devfs_tty_alias_read, devfs_tty_alias_write, NULL);
}