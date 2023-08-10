#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <new>
#include <stdint.h>

#define ALLOCATE malloc 
#define kb(s) (1024*(s))
#define mb(s) (1024*kb(s))
#define gb(s) (1024*mb(s))


struct RawBuffer{
    const char *id;
    void *base;
    int size;

    RawBuffer(const char *buffer_id, int in_size){
        base = ALLOCATE(sizeof(in_size));
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
};

template<typename T>
struct StackState{
    void *base;
    void *next;
    void *max;
    StackState(void *in_base, int size){
        base = in_base;
        next = base;
        int max_count = size / sizeof(T);
        max = (void *)((char *)base + max_count * sizeof(T));
    }
    bool Full(){
        return (next >= max);
    }
    bool Valid(){
        return (next > base && next <= max);
    }
};

template<typename T>
struct StackCursor{
    void *Push(void *loc, T dat){
        T *current = (T*)loc;
        *current = dat;
        return (void *)(++current);
    }
    T Pop(void **loc){
        T *val_ptr = (T*)*loc;
        val_ptr--;
        *loc = (void *)val_ptr;
        return *val_ptr;
    }
    T* Top(void *loc){
        return ((T *)loc)-1;
    }
};

template<typename T>
struct Node_sll{
    Node_sll *next;
    T data;
    Node_sll(T in_data):next(NULL), data(in_data){}
};

template<typename T>
struct ListState{
    Node_sll<T> *head;
    Node_sll<T> *current;
};

template<typename T>
struct LinkedList{
    Node_sll<T> *Next(Node_sll<T> *self){
        return self->next;
    }
    Node_sll<T> *Reverse(Node_sll<T> *self){
        Node_sll<T> *prev = self;
        Node_sll<T> *current = self->next;
        while(current){
            Node_sll<T> *next = current->next;
            current->next = prev;
            prev = current;
            current = next;
        }
        self->next = NULL;
    }

    void Append(Node_sll<T> *item, Node_sll<T> *list){
        item->next = list;
    }

    void InsertAfter(Node_sll<T> *at, Node_sll<T> *item){
        item->next = at->next;
        at->next = item;
    }

    Node_sll<T> *RemoveNext(Node_sll<T> *at){
        Node_sll<T> *item = at->next;
        at->next = item->next;
        item->next = NULL;
        return item;
    }

    Node_sll<T> *Calve(Node_sll<T> **list){
        Node_sll<int> *free = *list;
        *list = free->next;
        free->next = NULL;
        return free;
    }

    int Count(Node_sll<T> *list){
        if(!list){  
            return 0; 
        }
        Node_sll<T> *current = list;
        int count = 1;
        while((current = Next(current)) != NULL){
            count++;
        }
        return count;
    }

    void PrintContents(Node_sll<T> *list, void(*print_fn)(T)){
        int max_length = 16;
        int length = 0;
        if(!list){  
            return; 
        }
        Node_sll<T> *current = list;
        do{
            length++;
            print_fn(current->data);
        }
        while((current = Next(current)) && length < max_length);
        printf("X\n");
    }
};

template<typename T>
struct SLLAllocator{
    Node_sll<T> *free_list;
    Node_sll<T> *used_list;
    RawBuffer *mem;
    SLLAllocator(RawBuffer *in_mem){
        mem = in_mem;
        free_list = (Node_sll<T> *)in_mem->base;
        used_list = NULL;
        int count = mem->size/sizeof(Node_sll<T>);
        for(int i = 0; i < count-1; ++i){
            free_list[i].next = &free_list[i+1];
            free_list[i].data= i;
        }
        free_list[count-1].data = count-1;
        free_list[count-1].next = NULL;
    }

    T* Allocate(){
        if(free_list == NULL){
            return NULL;
        }
        LinkedList<T> cursor;
        Node_sll<T> *front = cursor.Calve(&free_list);
        if(used_list){
            cursor.Append(front, used_list);
        }
        used_list = front;
        return &used_list->data;
    }

    int Free(T *item){
        int result = 0;
        LinkedList<T> cursor;
        if(mem->InRange((item))){
            Node_sll<T> *node = (Node_sll<T> *)((char *)item - sizeof(void *));
            if(node == used_list){
                node = cursor.Calve(&used_list);
            }
            else{
                Node_sll<T> *current = used_list;
                while(current->next != node){
                    current = current->next;
                }
                node = cursor.RemoveNext(current);
            }
            cursor.Append(node, free_list);
            free_list = node;
            result = 1;
        }
        else{
            result = 0;
        }
        return result;
    }

    void Print(void(*print_fn)(T)){
        LinkedList<T> cursor;
        Node_sll<T> *current = free_list;
        printf("Free Nodes:\n");
        cursor.PrintContents(free_list, print_fn);
        printf("Used Nodes:\n");
        cursor.PrintContents(used_list, print_fn);
    } 
};

void int_allocator_print(int data){
    printf("%d->", data);
}


struct ArenaHeader{
    int size;
    int prev;
};

struct Arena{
    RawBuffer *buffer;
    ArenaHeader *top;

    Arena(RawBuffer *in_mem){
        buffer = in_mem;
        top = (ArenaHeader *)buffer->base;
        top->prev = 0;
    }

    ArenaHeader *Push(int arena_size_bytes){
        int allocation_size = ((arena_size_bytes + 7) / 8) * 8; //round allocation to nearest 64 bytes

        ArenaHeader *item = top;
        item->size=allocation_size;
        char *arena_top_bytes = (char*)top;
        arena_top_bytes += allocation_size + sizeof(ArenaHeader);

        if(buffer->InRange((void *)arena_top_bytes)){
            top = (ArenaHeader *)arena_top_bytes;
            top->prev = (int)(top - item);
            
            return item;
        }
        else{
            return NULL;
        }
    }

    void Pop(){
        if(top->prev == 0){
            //already at base
        }
        else{
            char *arena_top_bytes = (char *)top;
            arena_top_bytes -= top->prev;
            top = (ArenaHeader *)arena_top_bytes;

            assert(buffer->InRange((void *)top));
        }
    }

    void Print(){
        ArenaHeader *current = top;
        printf("Printing Arena Allocations\n");
        while(current->prev != 0){
            printf("Arena [Size|Prev]: [%d|%d]", current->size, current->prev);
            current = (ArenaHeader *)(((char *)current) - current->prev);
            if(!buffer->InRange((void *)current)){
                printf("Error: Traversal down stack invalid\n");
                break;
            }
        }
    }
};

int main(){
    RawBuffer stack_memory("Stack memory", 128);
    StackState<int> state(stack_memory.base,stack_memory.size);
    StackCursor<int> stack_cursor;

    int count = 0;
    while(!state.Full()){
        state.next = stack_cursor.Push(state.next,count++);
    }
    while(state.Valid()){
        printf("Next stack val: %d\n", stack_cursor.Pop(&state.next));
    }
    printf("Count traversed: %d\n", count);
    printf("next stack val: %d\n", *(int *)state.base);

    RawBuffer list_memory("List Memory", sizeof(Node_sll<int>) * 16);
    

    RawBuffer arena_memory("Arena memory", 512);
    Arena arena(&arena_memory);

    arena.Print();
    ArenaHeader *allocation_1 = arena.Push(8);
    ArenaHeader *allocation_2 = arena.Push(12);
    ArenaHeader *allocation_3 = arena.Push(16);
    ArenaHeader *allocation_4 = arena.Push(4);
    printf("Allocation 1 location: %p\n", allocation_1);
    printf("Allocation 2 location: %p\n", allocation_2);
    printf("Allocation 3 location: %p\n", allocation_3);
    printf("Allocation 4 location: %p\n", allocation_4);
    printf("2-1: %ld\n", allocation_2 - allocation_1);
    printf("3-2: %ld\n", allocation_3 - allocation_2);
    printf("4-3: %ld\n", allocation_4 - allocation_3);
    printf("top-4: %ld\n", arena.top - allocation_4);
    arena.Print();

    return 0;
}