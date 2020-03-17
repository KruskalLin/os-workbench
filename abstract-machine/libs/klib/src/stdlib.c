//
// Created by Popping Lim on 2020/3/14.
//

#include "klib.h"

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)


static unsigned long int next = 1;

static inline int atoi8(const char* nptr){
    int result = 0;
    for(; *nptr; ++nptr){
        result *= 8;
        result += *nptr - '0';
    }
    return result;
}

static inline int atoi10(const char* nptr){
    int result = 0;
    for(; *nptr; ++nptr){
        result *= 10;
        result += *nptr - '0';
    }
    return result;
}

static inline int atoi16(const char* nptr){
    int result = 0;
    for(; *nptr; ++nptr){
        result *= 16;
        if(*nptr >= '0' && *nptr <= '9'){
            result += *nptr - '0';
        } else if(*nptr >= 'a' && *nptr <= 'z'){
            result += *nptr - 'a' + 10;
        } else if(*nptr >= 'A' && *nptr <= 'Z'){
            result += *nptr - 'A' + 10;
        }
    }
    return result;
}

int atoi(const char* nptr){
    if(nptr[0] == '0'){
        if(nptr[1] == 'x'){
            return atoi16(nptr + 2);
        } else{
            return atoi8(nptr + 1);
        }
    } else{
        switch(nptr[0]){
            case '-':
                return -atoi10(nptr + 1);
            case '+':
                ++nptr;
            default:
                return atoi10(nptr);
        }
    }
}

int abs(int x) {
    return x >= 0 ? x : -x;
}

int rand(void) {
    // RAND_MAX assumed to be 32767
    next = next * 1103515245 + 12345;
    return (unsigned int)(next / 65536) % 32768;
}

void srand(unsigned int seed) {
    next = seed;
}


// TODO
unsigned long time() {
    return -1;
}


#endif