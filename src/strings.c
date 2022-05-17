#include <stdarg.h>
#include <stdint.h>
#include "malloc.h"
#include "strings.h"

#define ONE_E_19 10000000000000000000ull

/*

  %16h : print 8 bytes (16 characters)  (I guess we can just zero-pad for anything over 16)?
  %7h  : print 3 1/2 bytes (7 characters)
  %1h  : print 1 nibble (1 character)
  etc.

  I guess I can just call QwordToHex for all, and if width specifier is greater than 16, add zeros before
    appending string.  And appending can look the same for everything:
      M_append(s, qs+16-width)
      (So if width is 16, we append qs exactly as we curently do.
       If width is 1, we append qs+15, meaning the last character before the '\0'.
       And it even works for %0h, which in this system means to take up zero characters -- we'll append the
         string starting at '\0', leaving just s, so we're appending nothing.  We'll have copied s for no
         reason, but the default behavior would be correct, so this seems right.

  So, at least for hex, the number between '%' and '%' is how many characters wide to make the hex.

  At some point I'll likely want to space-pad decimal numbers or strings, but this seems good for now.

 */

#define nibbleToHex(n) n > '9' ? n + 'A' - '9' - 1 : n

static inline void byteToHex(uint8_t b, char* s) {
    char bh = (b >> 4) + '0';
    char bl = (b & 0x0f) + '0';
    s[0] = nibbleToHex(bh);
    s[1] = nibbleToHex(bl);
}

static inline void wordToHex(uint16_t w, char* s) {
    uint8_t b = (uint8_t) (w >> 8);
    byteToHex(b, s);
    b = (uint8_t) w;
    byteToHex(b, s+2);
}

static inline void dwordToHex(uint32_t d, char* s) {
    uint16_t w = (uint16_t) (d >> 16);
    wordToHex(w, s);
    w = (uint16_t) d;
    wordToHex(w, s+4);
}

static inline void qwordToHex(uint64_t q, char* s) {
    uint32_t d = (uint32_t) (q >> 32);
    dwordToHex(d, s);
    d = (uint32_t) q;
    dwordToHex(d, s+8);
}

static inline char* append(char* s, char* t) {
    char* u = M_append(s, t);
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
    int scap = 64;
    char* s = malloc(scap);
    char c, *t;
    int i = 0; // Index into s where we will place next character
    int width, padw;
    char buf[21];
    buf[20] = 0;

    for (char* p = fmt; *p; p++) {
        if (scap - i < 2) {
            scap *= 2;
            s = realloc(s, scap);
        }

        if (*p != '%') {
            s[i++] = *p;
            continue;
        }

        width = -1;
        padw = 0;

        c = *++p; // Skip past the '%'

        if (c >= '0' && c <= '9') {
            width = dstoui(p);
            while (*p >= '0' && *p <= '9')
                p++;
            c = *p;
        }

        switch (c) {
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
            t = buf+4; // Quad word in hex is 16 characters, not 20 as in decimal
            qwordToHex(va_arg(ap, uint64_t), t);
            break;
        case 's':
            t = va_arg(ap, char*);
            break;
        }

        // Okay, for now let's say width means: pad with zeros if necessary, and drop chars from left if necessary.
        if (c == 'u' || c == 'h') {
            int l = strlen(t);

            if (width < 0)
                while (*t == '0' && *(t+1) != 0) t++;
            else if (width < l)
                t += l - width;
            else
                padw = width - l;
        }

        // Okay, no append for s; here is the one place we lengthen it other than the top.
        int needed = i + strlen(t) + padw + 1;
        if (scap < needed) {
            scap = needed;
            s = realloc(s, scap);
        }

        for (int j = 0; j < padw; j++)
            s[i++] = '0';

        while (*t)
            s[i++] = *t++;
    }

    s[i] = '\0';

    return s;
}

int strlen(char* s) {
    int i = 0;
    while (s[i]) i++;
    return i;
}

char* M_append(char* s, char* t) {
    char* u = malloc(strlen(s) + strlen(t) + 1);

    char* up = u;
    while(*s)
        *up++ = *s++;
    while(*t)
        *up++ = *t++;

    *up = 0;
    return u;
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