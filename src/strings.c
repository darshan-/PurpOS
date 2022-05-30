#include <stdarg.h>
#include <stdint.h>

#include "strings.h"

#include "malloc.h"

#define ONE_E_19 10000000000000000000ull

static inline char* append(char* s, char* t) {
    char* u = M_sappend(s, t);
    free(s);
    return u;
}

char* M_sprintf(char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    char* s = M_vsprintf(fmt, ap);
    va_end(ap);
    return s;
}

char* M_vsprintf(char* fmt, va_list ap) {
    int scap = 128;
    char* s = malloc(scap);
    char c, *t, padc;
    int i = 0; // Index into s where we will place next character
    int padw = 0;
    char buf[21];
    buf[20] = 0;

    for (char* p = fmt; *p; p++) {
        if (scap - i < 2) {
            scap *= 2;
            s = realloc(s, scap);
        }

        if (padw == 0) {
            if (*p != '%') {
                s[i++] = *p;
                continue;
            }

            c = *++p; // Skip past the '%'
        } else {
            c = *p;
        }

        switch (c) {
        case 'c':
            t = buf + 19;
            t[0] = va_arg(ap, int);

            break;
        case 'u':
            uint64_t u = va_arg(ap, uint64_t);
            uint64_t e = ONE_E_19;

            for (int j = 0; j < 20; j++) {
                uint64_t d = u / e;
                buf[j] = d + '0';
                u  = u % e;
                e /= 10;
            }

            t = buf;

            break;
        case 'h':
            t = buf + 4; // Quad word in hex is 16 characters, not 20 as in decimal
            qwordToHex(va_arg(ap, uint64_t), t);

            break;
        case 's':
            t = va_arg(ap, char*);

            break;
        case 'p':
            padc = *++p;

            c = *++p;
            if (c >= '0' && c <= '9') {
                padw = dstoui(p);

                while (*p >= '0' && *p <= '9')
                    p++;

                p--;
            } else { // Bad format; let's bail on interpreting it
                s[i++] = '%';
                s[i++] = 'p';
                s[i++] = c;
            }

            continue;

            break;
        case '%':
            s[i++] = c;
            continue;
            break;
        default:
            s[i++] = '%';
            s[i++] = c;

            continue;
        }

        if (c == 'u' || c == 'h')
            if (padc != '0' || padw <= 0)
                while (*t == '0' && *(t + 1) != 0) t++;

        if (padw > 0) {
            int l = strlen(t);

            if (padw >= l) {
                padw -= l;
            } else {
                if (c == 'u' || c == 'h')
                    t += l - padw; // Keep the zeros we want rather than stripping and re-adding
                padw = 0;
            }
        }

        // Okay, no append for s; here is the one place we lengthen it other than the top.
        int needed = i + strlen(t) + padw + 1;
        if (scap < needed) {
            scap = needed;
            s = realloc(s, scap);
        }

        for (int j = 0; j < padw; j++)
            s[i++] = padc;

        while (*t)
            s[i++] = *t++;

        padw = 0;
    }

    s[i] = '\0';

    return s;
}

uint64_t strlen(char* s) {
    uint64_t i = 0;
    while (s[i]) i++;
    return i;
}

char* M_sappend(char* s, char* t) {
    char* u = malloc(strlen(s) + strlen(t) + 1);

    char* up = u;
    while(*s)
        *up++ = *s++;
    while(*t)
        *up++ = *t++;

    *up = 0;
    return u;
}

char* M_scopy(char* s) {
    char* t = malloc(strlen(s) + 1);

    char* tp = t;
    while(*s)
        *tp++ = *s++;

    *tp = 0;
    return t;
}

// Decimal string to unsigned int
uint64_t dstoui(char* s) {
    uint64_t i = 0;
    for (; *s >= '0' && *s <= '9'; s++) {
        i *= 10;
        i += *s - '0';
    }
    return i;
}
