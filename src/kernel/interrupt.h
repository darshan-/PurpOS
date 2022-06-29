#pragma once

#include <stdint.h>

#include "log.h"

void init_interrupts();
void waitloop();
void* startApp(uint64_t stdout);

extern uint64_t int_blocks;
extern uint64_t* kernel_stack_top;

static inline void no_ints() {
    __asm__ __volatile__("cli");
    int_blocks++;
}

static inline void ints_okay_once_on() {
    if (int_blocks <= 0) {
        logf("WARNING: ok_ints() called when int_blocks was %u... You have a bug.\n", int_blocks);
        return;
    }

    int_blocks--;
}

static inline void ints_okay() {
    ints_okay_once_on();

    if (int_blocks == 0)
        __asm__ __volatile__("sti");
}