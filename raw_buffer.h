#pragma once

#define ALLOCATE malloc 
#define FREE free
#define kb(s) (1024*(s))
#define mb(s) (1024*kb(s))
#define gb(s) (1024*mb(s))


struct RawBuffer{
    const char *id;
    void *base;
    int size;

    RawBuffer(const char *buffer_id, int in_size){
        base = ALLOCATE(in_size);
        size = in_size;
        id = buffer_id;
    }

    RawBuffer(const char *buffer_id, int in_size, void *in_memory){
        base = in_memory;
        size = in_size;
        id = buffer_id;
    }

    void Print(){
        printf("RawBuffer[id][size][base]: [%s][%d][%p]\n", id, size, base);
    }

    int InRange(void *data){
            return (data >= base && data < (void *)((char *)base + size));
    }

    void Delete(){
        FREE(base);
    }
};