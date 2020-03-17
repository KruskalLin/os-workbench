//
// Created by Popping Lim on 2020/3/14.
//

#include "klib.h"
#include <stdarg.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)
#define MAX_LENGTH 200

int vsprintf(char* str, const char* fmt, va_list ap) {
    char* p_str = str;
    const char* p_format = fmt;
    for(; *p_format; p_format++) {
        if(*p_format != '%') {
            (*p_str++) = *p_format;
        } else {
            // TODO
//            int width, prec, length, flag=-1;
            p_format++;
            char* s;
            switch (*p_format) {
                case 's':
                    s = va_arg(ap, char*);
                    while(*s)
                        *p_str++ = *s++;
                    break;
                default:;
            }
        }
    }
    *p_str = '\0';
    return p_str - str;
}

int printf(const char* fmt, ...) {
    char output[MAX_LENGTH];
    va_list ap;
    va_start(ap, fmt);
    int len = vsprintf(output, fmt, ap);
    va_end(ap);
    int i = 0;
    while(output[i]){
        _putc(output[i]);
        i++;
    }
    return len;
}


int sprintf(char* out, const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int len = vsprintf(out, fmt, ap);
    va_end(ap);
    return len;
}


// TODO
int snprintf(char* s, size_t n, const char* format, ...) {
    return 0;
}
int vsnprintf(char* str, size_t size, const char *format, va_list ap) {
    return 0;
}
int sscanf(const char* str, const char *format, ...) {
    return 0;
}
void qsort(void* base, size_t nmemb, size_t size, int (*compar)(const void *, const void *)) {}


#endif