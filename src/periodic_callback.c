#include <stdint.h>
#include "list.h"
#include "malloc.h"
#include "periodic_callback.h"
#include "periodic_callback_int.h"

#define INIT_CAP 10

struct periodic_callbacks periodicCallbacks = {0, 0};

static uint64_t cap = 0;

void registerPeriodicCallback(struct periodic_callback c) {
    __asm__ __volatile__("cli");

    if (!periodicCallbacks.pcs) {
        cap = INIT_CAP;
        periodicCallbacks.pcs = malloc(INIT_CAP * sizeof(void*));
    } else if (periodicCallbacks.len + 1 >= cap) {
        periodicCallbacks.pcs = realloc(periodicCallbacks.pcs, cap * 2);
    }

    struct periodic_callback* cp = (struct periodic_callback*) malloc(sizeof(struct periodic_callback));
    cp->count = c.count;
    cp->period = c.period;
    cp->f = c.f;

    periodicCallbacks.pcs[periodicCallbacks.len++] = cp;

    __asm__ __volatile__("sti");
}

void unregisterPeriodicCallback(struct periodic_callback c) {
    if (!periodicCallbacks.pcs) return;

    __asm__ __volatile__("cli");

    int found = 0;
    for (uint64_t i = 0; i < periodicCallbacks.len - 1; i++) {
        if (found) {
            periodicCallbacks.pcs[i] = periodicCallbacks.pcs[i+1];
        } else if (periodicCallbacks.pcs[i]->count == c.count &&
                   periodicCallbacks.pcs[i]->period == c.period &&
                   periodicCallbacks.pcs[i]->f == c.f) {
            found = 1;
        }
    }

    periodicCallbacks.len--;

    __asm__ __volatile__("sti");

    // Remove callback that matches c from callbacks
    // removeFromListWithEquality(periodicCallbackList, ({
    //     int __fn__ (void* other) {
    //         return
    //             //((struct periodic_callback) other)->Hz == c.Hz &&
    //             ((struct periodic_callback*) other)->count == c.count &&
    //             ((struct periodic_callback*) other)->period == c.period &&
    //             ((struct periodic_callback*) other)->f == c.f;
    //     }

    //     __fn__;
    // }));
}
