#pragma once


struct ArenaHeader{
    int size;
    int prev;
};

struct Arena{
private:
    ArenaHeader *Previous(ArenaHeader *current){
        return (ArenaHeader *)(((char *)current) - current->prev);
    }
public:

    RawBuffer *buffer;
    ArenaHeader *top;

    Arena(RawBuffer *in_mem){
        buffer = in_mem;
        top = (ArenaHeader *)buffer->base;
        top->prev = 0;
    }

    ArenaHeader *Push(int arena_size_bytes){
        int allocation_size = ((arena_size_bytes + 7) / 8) * 8; //round allocation to nearest 64 bytes
        //printf("Arena allocation size: %d\n", allocation_size);
        ArenaHeader *item = top;
        item->size=allocation_size;
        char *arena_top_bytes = (char*)top;
        //printf("Arena Top before push: %ld\n", arena_top_bytes - (char *)buffer->base);
        arena_top_bytes += allocation_size + sizeof(ArenaHeader);
        //printf("Arena Top after push: %ld\n", arena_top_bytes - (char *)buffer->base);


        if(buffer->InRange((void *)arena_top_bytes)){
            top = (ArenaHeader *)arena_top_bytes;
            top->prev = (int)((char *)top - (char *)item);
            
            return item;
        }
        else{
            return NULL;
        }
    }

    void Pop(){
        if(top->prev == 0){
            //already at base
            top->size = 0;
        }
        else{
            char *arena_top_bytes = (char *)top;
            arena_top_bytes -= top->prev;
            top = (ArenaHeader *)arena_top_bytes;
            top->size = 0;

            assert(buffer->InRange((void *)top));
        }
    }

    void Print(){
        ArenaHeader *current = Previous(top);
        printf("Printing Arena Allocations\n\t[T");
        do{
            printf(" %d ", current->size);
            current = Previous(current);
            if(!buffer->InRange((void *)current)){
                printf("Error: Traversal down stack invalid\n");
                break;
            }
        }while(current->prev != 0);
        printf("B]\n");
    }
};