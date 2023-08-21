#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <new>
#include <stdint.h>
#include <string.h>
#include "raw_buffer.h"
#include "stack.h"
#include "single_linked_list.h"
#include "double_linked_list.h"
#include "double_linked_offset_list.h"
#include "arena.h"
#include "square_mat.h"
#include "adjacency_list.h"

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
struct ListState{
    Node_sll<T> *head;
    Node_sll<T> *current;
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
            cursor.Prepend(front, used_list);
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
            cursor.Prepend(node, free_list);
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

void PrintEdges(int i){
    printf("%d->",i);
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

    int node_count = 8;
    int edge_count = 10;
    RawBuffer adjacency_list_memory("Adjacency List Memory", sizeof(Node_sll<int>) * (node_count + edge_count));

    int node_ids[] = {0, 1, 2, 3, 4, 5, 6, 7};
    int edge_counts[] = {1, 2, 2, 2, 1, 1, 0, 1};
    int edges[] = {5, 0, 2, 4, 5, 7, 2, 5, 6, 3}; 

    AdjacencyList list(&adjacency_list_memory, node_count);
    list.InitList(node_ids,edge_counts, edges);
    list.Print(PrintEdges);


    return 0;
}