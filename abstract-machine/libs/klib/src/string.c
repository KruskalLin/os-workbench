//
// Created by Popping Lim on 2020/3/14.
//

#include "klib.h"

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

void *memset(void *v, int c, size_t n) {
    char *p = (char *) v;
    for (size_t i = 0; i < n; i++, p++) {
        *p = c;
    }
    return v;
}

void *memcpy(void *dst, const void *src, size_t n) {
    char *p_dst = (char *) dst;
    const char *p_src = (const char *) src;
    for (size_t i = 0; i < n; i++, p_dst++, p_src++) {
        *p_dst = *p_src;
    }
    return dst;
}

void *memmove(void *dst, const void *src, size_t n) {
    char *p_dst = (char *) dst;
    const char *p_src = (const char *) src;
    if (dst > src) {
        for (int i = n - 1; i >= 0; ++i) {
            p_dst[i] = p_src[i];
        }
    } else {
        for (int i = 0; i < n; ++i) {
            p_dst[i] = p_src[i];
        }
    }
    return dst;
}

int memcmp(const void *s1, const void *s2, size_t n) {
    const char *p_s1 = (const char *) s1;
    const char *p_s2 = (const char *) s2;
    for (size_t i = 0; i < n; i++, p_s1++, p_s2++) {
        if (*p_s1 != *p_s2) {
            return *p_s1 - *p_s2;
        }
    }
    return 0;
}

size_t strlen(const char *s) {
    size_t len = 0;
    const char *p = s;
    for (; *p; p++, len++) {}
    return len;
}

char *strcat(char *dst, const char *src) {
    char *p_dst = dst + strlen(dst);
    strcpy(p_dst, src);
    return dst;
}

char *strcpy(char *dst, const char *src) {
    return strncpy(dst, src, strlen(src));
}

char *strncpy(char *dst, const char *src, size_t n) {
    char *p_dst = (char *) dst;
    const char *p_src = (const char *) src;
    for (size_t i = 0; i < n; i++, p_dst++, p_src++) {
        *p_dst = *p_src;
    }
    *p_dst = '\0';
    return dst;
}

int strcmp(const char *s1, const char *s2) {
    size_t len1 = strlen(s1);
    size_t len2 = strlen(s2);
    size_t n = len1 < len2 ? len1 : len2;
    return strncmp(s1, s2, n + 1);
}

int strncmp(const char *s1, const char *s2, size_t n) {
    const char *p_s1 = s1;
    const char *p_s2 = s2;
    for (size_t i = 0; i < n; i++, p_s1++, p_s2++) {
        if (*p_s1 != *p_s2) {
            return *p_s1 - *p_s2;
        }
    }
    return 0;
}

// TODO
char *strtok(char *s, const char *delim) {
    return NULL;
}

char *strstr(const char *str1, const char *str2) {
    return NULL;
}

char *strchr(const char *s, int c) {
    return NULL;
}

char *strrchr(const char *s, int c) {
    return NULL;
}

#endif