#include <stdarg.h>
#include <stdint.h>

#include "sys.h"

#include "../lib/malloc.h"
#include "../lib/strings.h"
#include "../lib/syscall.h"

/*
   0: exit
   1: printf
   2: printColor
   3: readline
   4: runProg
   5: wait
   6: getProcs

  */

void exit() {
    asm volatile("\
\n      mov $0, %rax                            \
\n      int $0x80                               \
    ");
}

void wait(uint64_t p) {
    if (!p) // Good safety check, and also makes it easy to wait on runProg even if we're not sure if app exists (maybe return error code from here?)
        return;

    asm volatile("\
\n      mov $5, %%rax                           \
\n      mov %0, %%rbx                           \
\n      int $0x80                               \
    "::"m"(p));
}

uint64_t runProg(char* s) {
    uint64_t p;
    asm volatile("\
\n      mov $4, %%rax                           \
\n      mov %1, %%rbx                           \
\n      int $0x80                               \
\n      mov %%rax, %0                           \
    ":"=m"(p):"m"(s));

    return p;
}

void printColor(char* s, uint8_t c) {
    asm volatile("\
\n      mov $2, %%rax                           \
\n      mov %0, %%rbx                           \
\n      movb %1, %%cl                           \
\n      int $0x80                               \
    "::"m"(s),"m"(c));
}

void print(char* s) {
    printColor(s, 0x07);
}

void printf(char* fmt, ...) {
    VARIADIC_PRINT(print);
}

char* M_readline() {
    uint64_t len; // Kernel needs to know len in order to know where to put it on stack, so use that knowledge (to malloc needed size)
    char* l;

    asm volatile("\
\n      mov $3, %%rax                           \
\n      int $0x80                               \
\n      mov %%rax, %0                           \
\n      mov %%rbx, %1                           \
    ":"=m"(len), "=m"(l));

    char* s = malloc(len + 1);
    for (uint64_t i = 0; i < len; i++)
        s[i] = l[i];
    s[len] = 0;

    return s;
}

struct sc_proc* M_getProcs() {
    uint64_t size;
    struct sc_proc *procs;

    asm volatile("\
\n      mov $6, %%rax                           \
\n      int $0x80                               \
\n      mov %%rax, %0                           \
\n      mov %%rbx, %1                           \
    ":"=m"(size), "=m"(procs));

    struct sc_proc *p = malloc(size * sizeof(struct sc_proc));
    for (uint64_t i = 0; i < size; i++)
        p[i] = procs[i];

    return p;
}

uint64_t stdout;

extern void main();

void __attribute__((section(".entry")))  _entry() {
    // I think I might prefer to use linker to place map last in text section, and have heap grow up toward stack, and have stack at end
    //   of page...

    asm volatile("mov %%r15, %0":"=m"(stdout));
    init_heap((uint64_t*) 0x7FC0180000ull, 0x80000);
    main();
    exit();
}
