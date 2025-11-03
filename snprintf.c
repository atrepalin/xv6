#include "types.h"

int snprintf(char *buf, int size, const char *fmt, ...) {
    char *p = buf;
    char *end = buf + size - 1;
    const char *f = fmt;

    uint *argp = (uint*)(void*)(&fmt + 1);

    while (*f && p < end) {
        if (*f == '%') {
            f++;
            if (*f == 's') {
                char *s = (char*)*argp++;
                if (!s) s = "(null)";
                while (*s && p < end) *p++ = *s++;
            } else if (*f == 'd') {
                int val = (int)*argp++;
                char tmp[16];
                int i = 0;

                int neg = 0;
                if (val < 0) { neg = 1; val = -val; }

                if (val == 0) tmp[i++] = '0';
                else {
                    while (val > 0) {
                        tmp[i++] = '0' + (val % 10);
                        val /= 10;
                    }
                }
                if (neg) tmp[i++] = '-';

                for (int j = i - 1; j >= 0 && p < end; j--) *p++ = tmp[j];
            } else {
                if (p < end) *p++ = '%';
                if (p < end) *p++ = *f;
            }
            f++;
        } else {
            *p++ = *f++;
        }
    }

    *p = '\0';
    return p - buf;
}
