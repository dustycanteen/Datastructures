#pragma once


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