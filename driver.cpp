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
    Node_sll<T> *next;
    T data;
    Node_sll(T in_data):next(NULL), data(in_data){}
};

template<typename T>
struct ListState{
    Node_sll<T> *head;
    Node_sll<T> *current;
};

template<typename T>
struct SLinkedList{
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
        SLinkedList<T> cursor;
        Node_sll<T> *front = cursor.Calve(&free_list);
        if(used_list){
            cursor.Append(front, used_list);
        }
        used_list = front;
        return &used_list->data;
    }

    int Free(T *item){
        int result = 0;
        SLinkedList<T> cursor;
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
        SLinkedList<T> cursor;
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
        printf("Arena allocation size: %d\n", allocation_size);
        ArenaHeader *item = top;
        item->size=allocation_size;
        char *arena_top_bytes = (char*)top;
        printf("Arena Top before push: %ld\n", arena_top_bytes - (char *)buffer->base);
        arena_top_bytes += allocation_size + sizeof(ArenaHeader);
        printf("Arena Top after push: %ld\n", arena_top_bytes - (char *)buffer->base);


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


template <typename T>
struct Node_dll{
    Node_dll<T> *prev;
    Node_dll<T> *next;
    T data;
};

template <typename T>
struct DLinkedList{
    Node_dll<T> *Init(void *buffer, T in_data){
        Node_dll<T> *node = (Node_dll<T> *)buffer;
        node->prev = NULL;
        node->next = NULL;
        node->data = in_data;
        
        return node;
    }

    Node_dll<T> *Head(Node_dll<T> *item){
        const int MAX_LIST_SIZE = 16;
        int count = 0;

        Node_dll<T> *current = item;

        while((current = current->prev)->prev != NULL && count++ < MAX_LIST_SIZE); //just assume no cycles D:
        return current;
    }
    
    Node_dll<T> *Tail(Node_dll<T> *item){
        const int MAX_LIST_SIZE = 16;
        int count = 0;

        Node_dll<T> *current = item;

        while((current = current->next)->next != NULL && count++ < MAX_LIST_SIZE); //just assume no cycles D:
        return current;
    }
    
    void Insert(Node_dll<T> *item, Node_dll<T> *at){
        item->next = at->next;
        at->next = item;
        item->prev = at;
        assert(at != item);
        if(item->next){
            item->next->prev = at;
        }
    }

    void Remove(Node_dll<T> *item){
        if(item->next)
        {
            item->next->prev = item->prev;
        }
        if(item->prev){
            item->prev->next = item->next;
        }
        item->next = NULL;
        item->prev = NULL;
    }

    void PrintFrom(Node_dll<T> *item){
        const int MAX_LIST_SIZE = 16;
        int count = 0;

        printf("Printing List From Node: %p\n", (void *)item);
        printf("[Node at: %p < %lld >] -> \n", (void *)item, item->data);
        Node_dll<T> *current = item;
        while((current = current->next)->next != NULL && count++ < MAX_LIST_SIZE){ //just assume no cycles D:
            printf("[Node at: %p < %lld >] -> \n", (void *)current, current->data);
        }
        printf("[Node at: %p < %lld >] -> \n", (void *)current, current->data);
    }
};

template <typename T>
struct Node_dlol{
    uint32_t offset_prev;
    uint32_t offset_next;
    T data;
};

template <typename T>
struct DLOffsetList{
    Node_dlol<T> *Init(void *memory, T in_data){
        Node_dlol<T> *item = (Node_dlol<T> *)memory;
        item->offset_prev = 0;
        item->offset_next = 0;
        item->data = in_data;

        return item;
    }

    void InsertFirstAfter(Node_dlol<T> *item, Node_dlol<T> *at){
        int tmp_next = at->offset_next;
        
        at->offset_next = item - at; 
        item->offset_prev = at - item;

        printf("Inserting node at %p after node at: %p | Computed Offsets: [at->next %d][item->prev %d]\n", item, at, at->offset_next, item->offset_prev);

        if(tmp_next){
            Node_dlol<T> *next = at + tmp_next;
            item->offset_next = next - item;
            next->offset_prev = item - next;
        }
        else{
            item->offset_next = 0;
        }
    }
    
    void InsertFirstBefore(Node_dlol<T> *item, Node_dlol<T> *at){
        int tmp_prev = at->offset_prev;

        at->offset_prev = item - at; 
        item->offset_next = at - item;

        if(tmp_prev){
            Node_dlol<T> *prev = at + tmp_prev;
            item->offset_prev = prev - item;
            prev->offset_prev = item - prev;
        } 
        else{
            item->offset_prev = 0;
        }
    }

    void Remove(Node_dlol<T> *item){
        Node_dlol<T> *next = item + item->offset_next;
        Node_dlol<T> *prev = item + item->offset_prev;

        next->offset_prev += item->offset_prev;
        prev->offset_next += item->offset_next;
        item->next = 0;
        item->prev = 0;
    }

    void Swap(Node_dlol<T> *A, Node_dlol<T> *B){
        int A_minus_B = A-B;
        int B_minus_A = -A_minus_B;
        
        int tmp_A_prev = A->offset_prev;
        int tmp_A_next = A->offset_next;
        A->offset_prev = A_minus_B + B->offset_prev;
        A->offset_next = A_minus_B + B->offset_next;
        B->offset_prev = B_minus_A + tmp_A_prev;
        B->offset_next = B_minus_A + tmp_A_next;
    }

    inline Node_dlol<T>* Next(Node_dlol<T> *item){
        if(item->offset_next != 0){
            return item+item->offset_next;
        }
        return NULL;
    }
    inline Node_dlol<T>* Prev(Node_dlol<T> *item){
        if(item->offset_prev != 0){
            return item+item->offset_prev;
        }
        return NULL;
    }
    
    void PrintFrom(Node_dlol<T> *item){
        Node_dlol<T> *current = item;
        printf("Printing from item at location: %p\n", current);
        do{
            printf("%lld -> ", current->data);
        }
        while((Next(current))!=NULL);
        printf("X\n");
    }
};

int main(){
    RawBuffer dll_memory("DLL memory", sizeof(Node_dll<int64_t>) * 16);
    DLinkedList<int64_t> cursor;
    Node_dll<int64_t> *list_head = cursor.Init(dll_memory.base, 0);
    for(int i = 1; i <= 10; ++i){
        cursor.Insert(cursor.Init(list_head+i,i), list_head+i-1);
    }
    Node_dll<int64_t> *list_tail = list_head + 10;
    printf("List head [prev][next]:  [%p][%p]\n", (void *)list_head->prev, (void *)list_head->next);
    cursor.PrintFrom(list_head);
    printf("List tail [prev][next]:  [%p][%p]\n", (void *)list_tail->prev, (void *)list_tail->next);

    printf("[Cached Head][Computed Head]:[%lld][%lld]\n", list_head->data, cursor.Head(list_tail)->data);
    printf("[Cached Tail][Computed Tail]:[%lld][%lld]\n", list_tail->data, cursor.Tail(list_head)->data);
    

    //Offset list is still broken
    //need to figure out why the olist head is still getting +16 for the offset
    RawBuffer offset_list("OffsetList memory", sizeof(Node_dlol<int64_t>) * 16);
    DLOffsetList<int64_t> offset_list_cursor;
    Node_dlol<int64_t> *olist_head = offset_list_cursor.Init(offset_list.base, 0);
    printf("OffsetList Node [ prev <- |Location| -> next ]\n");
    printf("[ %d <- |%p| -> %d ]\n", olist_head->offset_prev, olist_head, olist_head->offset_next);
    for(int i = 1; i < 16; ++i){
        offset_list_cursor.InsertFirstAfter(offset_list_cursor.Init(olist_head+i, i), olist_head+i-1);
    }
    //offset_list_cursor.PrintFrom(olist_head);
    printf("item next|prev: %d|%d\n", (olist_head+5)->offset_prev, (olist_head+5)->offset_next);
    printf("item next|prev: %d|%d\n", (olist_head+2)->offset_prev, (olist_head+2)->offset_next);

    offset_list_cursor.Swap(olist_head+5, olist_head+2);
    printf("item next|prev: %d|%d\n", (olist_head+5)->offset_prev, (olist_head+5)->offset_next);
    printf("item next|prev: %d|%d\n", (olist_head+2)->offset_prev, (olist_head+2)->offset_next);

    offset_list_cursor.Swap(olist_head+1, olist_head+14);
    offset_list_cursor.Swap(olist_head+14, olist_head+9);
    offset_list_cursor.Swap(olist_head+12, olist_head+6);
    offset_list_cursor.Swap(olist_head+2, olist_head+3);
    offset_list_cursor.Swap(olist_head+13, olist_head+7);
    offset_list_cursor.Swap(olist_head+4, olist_head+5);
    offset_list_cursor.Swap(olist_head+6, olist_head+11);
    offset_list_cursor.Swap(olist_head, olist_head+15);
    olist_head = olist_head+15;
    printf("list head next offset: %d\n", olist_head->offset_next);

    //offset_list_cursor.PrintFrom(olist_head);




    return 0;
}