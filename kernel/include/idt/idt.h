#pragma once
#include <stdint.h>

void idt_init();
void idt_set_descriptor(uint8_t vector, void *isr, uint8_t flags);