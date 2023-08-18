#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <new>
#include <stdint.h>
#include <string.h>

#define ALLOCATE malloc 
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
struct Stack{
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
    Node_sll(T in_data, Node_sll<T> *next):next(NULL), data(in_data){}

    operator bool() const{
        return next || data; 
    }
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

    operator bool() const{
        return prev || next || data; 
    }
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

    operator bool() const{
        return offset_prev || offset_next || data; 
    }
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

    Node_dlol<T>* Next(Node_dlol<T> *item){
        if(item->offset_next != 0){
            //May be a bug in the GCC compiler, if I don't cache both offset_next and the next node (both of them; using the cached offset to compute the position of the next node as well)
            //The computed node is off by 1mb (2^10). Only applies when the offset is negative.
            int offset_next = item->offset_next;  
            Node_dlol<T> *next_node = item+offset_next;
            
            return next_node;
        }
        return NULL;
    }
    Node_dlol<T>* Prev(Node_dlol<T> *item){
        if(item->offset_prev != 0){
            //May be a bug in the GCC compiler, if I don't cache both offset_next and the next node (both of them; using the cached offset to compute the position of the next node as well)
            //The computed node is off by 1mb (2^10). Only applies when the offset is negative.

            int offset_prev = item->offset_prev;
            Node_dlol<T> *prev_node = item+offset_prev;
            return prev_node;
        }
        return NULL;
    }

    void InsertAfter(Node_dlol<T> *item, Node_dlol<T> *prev){
        int tmp_next = prev->offset_next;
        
        prev->offset_next = item - prev; 
        item->offset_prev = prev - item;

        //printf("Inserting node at %p after node at: %p | Computed Offsets: [prev->next %d][item->prev %d]\n", item, prev, prev->offset_next, item->offset_prev);

        if(tmp_next){
            Node_dlol<T> *next = prev + tmp_next;
            item->offset_next = next - item;
            next->offset_prev = item - next;
        }
        else{
            item->offset_next = 0;
        }
    }
    
    void InsertBefore(Node_dlol<T> *item, Node_dlol<T> *next){
        int tmp_prev = next->offset_prev;
        
        next->offset_prev = item - next; 
        item->offset_next = next - item;
        if(tmp_prev){
            Node_dlol<T> *prev = next + tmp_prev;
            item->offset_prev = prev - item;
            prev->offset_next = item - prev;
        } 
        else{
            item->offset_prev = 0;
        }
    }

    Node_dlol<T> *Remove(Node_dlol<T> *item){
        int tmp_prev = item->offset_prev;
        int tmp_next = item->offset_next;
        if(tmp_prev && tmp_next){
            Node_dlol<T> *next = Next(item);
            Node_dlol<T> *prev = Prev(item);
            next->offset_prev += tmp_prev;
            prev->offset_next += tmp_next;
        }
        else{
            Node_dlol<T> *unsafe_prev = item + tmp_prev;
            Node_dlol<T> *unsafe_next = item + tmp_next;
            unsafe_prev->offset_next = 0;
            unsafe_next->offset_prev = 0;
        }
        return item;
    }
    
    void PrintFrom(Node_dlol<T> *item){
        Node_dlol<T> *current = item;
        printf("Printing from item at location: %p\n", current);
        do{
            printf("%lld -> ", current->data);
        }
        while((current = Next(current))!=NULL);
        printf("X\n");
    }
};

template<typename T>
struct Mat_nxn{
    size_t Footprint(int pitch){
        return sizeof(T) * pitch * pitch;
    }

    T *Row(void *base, int pitch, int row){
        if(row > pitch){
            return NULL;
        }
        T* accessor = (T*)base; 
        return accessor + row*pitch;
    }

    T *Cell(void *base, int pitch, int row, int column){
        if(row > pitch || column > pitch){
            return NULL;
        }
        T* accessor = (T*)base;
        return accessor + row*pitch + column;
    }

    void Zero(void *base, int pitch){
        memset(base,0,Footprint(pitch));
    }

    int CountEdges(void *base, int pitch){
        int count = 0;
        printf("Size of type: %lu\n", sizeof(T));
        for(int j = 0; j < pitch; ++j){
            for(int i = 0; i < pitch; i++){
                T *cell = Cell(base, pitch, j, i);
                int is_edge = !!(*cell);
                if(is_edge){
                    printf("Edge detected with value %d at row, column: %d,%d\n", is_edge, j, i);
                }
                count += is_edge;
            }
        }
        return count;
    }
};

void PrintIntMatrix(void *base, int pitch){
    printf("Int Matrix: \n");
    int *accessor = (int *)base;
    for(int j = 0; j < pitch; ++j){
        for(int i = 0; i < pitch; ++i){
            int item = accessor[j*pitch + i];
            if(item){
                printf("[O]");
            }
            else{
                printf("[ ]");
            }
        }
        printf("\n");
    }
}

void PrintListMatrix(void *base, int pitch){
    printf("List Matrix:\n");
    Node_dlol<int> *accessor = (Node_dlol<int> *)base;
    for(int j = 0; j < pitch; ++j){
        for(int i = 0; i < pitch; ++i){
            int item = accessor[j*pitch + i].data;
            if(item){
                printf("[O]");
            }
            else{
                printf("[ ]");
            }
        }
        printf("\n");
    }
}


int main(){
    Mat_nxn<int> int_matrix;
    Mat_nxn< Node_dlol<int> > list_matrix;
    int pitch = 8;
    RawBuffer int_matrix_memory("Int Matrix Memory", int_matrix.Footprint(pitch));
    RawBuffer list_matrix_memory("List Matrix Memory", list_matrix.Footprint(pitch));

    printf("Int Buffer base: %p", int_matrix_memory.base);
    printf("List Buffer base: %p", list_matrix_memory.base);

    list_matrix.Zero(list_matrix_memory.base, pitch);
    int_matrix.Zero(int_matrix_memory.base, pitch);
    
    PrintIntMatrix(int_matrix_memory.base, pitch);

    //printf("List Matrix computed edge count: %d\n",list_matrix.CountEdges(list_matrix_memory.base, pitch));
    //printf("Int Matrix computed edge count: %d\n",int_matrix.CountEdges(int_matrix_memory.base, pitch));
    
    for(int i = 0; i < 8; ++i){
        Node_dlol<int> *cell = list_matrix.Cell(list_matrix_memory.base, pitch, i,i);
        if(list_matrix_memory.InRange(cell)){
            cell->data = 1;
        }
        else{
            printf("Incorrectly calculated location for cell: (%d,%d)", i,i);
        }
    }
    
    printf("Int Memory  [Start -> End]: [%p -> %p]\n", (int *)int_matrix_memory.base, (int*)int_matrix_memory.base + int_matrix.Footprint(pitch));
    printf("Node Memory [Start -> End]: [%p -> %p]\n", (Node_dlol<int> *)list_matrix_memory.base, (Node_dlol<int> *) list_matrix_memory.base + list_matrix.Footprint(pitch));

    for(int i = 0; i < 8; ++i){
        int *cell = int_matrix.Cell(int_matrix_memory.base, pitch, i,i);
        if(int_matrix_memory.InRange(cell)){
            *cell = 1;
        }
        else{
            printf("Incorrectly calculated location for cell: (%d,%d)", i,i);
        }
    }
    
    printf("\n\n");
    PrintIntMatrix(int_matrix_memory.base, pitch);
    PrintListMatrix(list_matrix_memory.base, pitch);
    printf("List Matrix computed edge count: %d\n",list_matrix.CountEdges(list_matrix_memory.base, pitch));
    printf("Int Matrix computed edge count: %d\n",int_matrix.CountEdges(int_matrix_memory.base, pitch));

    int num_nodes = 8;
    int num_edges = 15;
    Stack< Node_sll<int> >adjacency_list;
    RawBuffer graph_data("Adjacency List Memory", (num_nodes + num_edges) * sizeof(Node_sll<int>));
    void *free = graph_data.base;
    for(int i = 0; i < num_nodes; ++i){
        free = adjacency_list.Push(free, Node_sll<int>(0));
    }
    Node_sll<int> *base = (Node_sll<int> *)graph_data.base;
    for(int i = 0; i < num_nodes; ++i){
        switch(i){
            case 0:{
                free = adjacency_list.Push(free, Node_sll<int>(5, ))
            }break;
            case 1:{

            }break;
            case 2:{

            }break;
            case 3:{

            }break;
            case 4:{

            }break;
            case 5:{

            }break;
            case 6:{

            }break;
            case 7:{

            }break;
            default:
            break;
        }
    }


    return 0;
}