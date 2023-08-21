#pragma once


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