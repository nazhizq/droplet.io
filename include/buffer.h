
#pragma once

#include <memory>
#include <iostream>
#include <string.h>

namespace sparrow {

typedef class Buf {
private:
    char*  base;
    size_t len;
public:
    Buf() {
        base = NULL;
        len  = 0;
    }
    Buf(size_t cap) : base(NULL), len(0) { 
        resize(cap); 
    }

    virtual ~Buf() {
        clean();
    }
    
    void*  data() { return base; }
    size_t size() { return len; }
    void clean() {
        free(base);
        base = NULL;
        len = 0;
    }
    bool isNull() { return base == NULL || len == 0; }
    void resize(size_t newsize) {
        if (newsize == len) return;
        if (base == NULL) {
            base = (char*)malloc(newsize);
        } else {
            void* ptr = (char*)realloc(base, newsize);
            if (newsize > len) {
                memset((char*)ptr + len, 0, newsize - len);
            }
            base = (char*)ptr;
        }
        len = newsize;
    }
    void copy(void* data, size_t len) {
        resize(len);
        memcpy(base, data, len);
    }
} Buffer;

}