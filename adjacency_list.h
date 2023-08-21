#pragma once

struct AdjacencyList{
private: 
    void InitEdges(int node_index, int *connection_data, int total_edge_count, int edge_count){
        Stack< Node_sll<int> >stack_ops;
        SLinkedList<int> linked_list_ops;

        Node_sll<int> *node = nodes + node_index;
        Node_sll<int> *edge = edges + total_edge_count;
        for(int i = 0; i < edge_count; ++i){
            Node_sll<int> *current = edge;
            edge = (Node_sll<int> *)stack_ops.Push(current, Node_sll<int>(connection_data[i]));
            linked_list_ops.Prepend(current, node->next);
            node->next = current;
        }
    }
public:
    RawBuffer *element_memory;
    int edge_count;
    int node_count;
    Node_sll<int> *nodes;
    Node_sll<int> *edges;

    AdjacencyList(RawBuffer *in_memory, int in_node_count){
        element_memory = in_memory;
        node_count = in_node_count;
        edge_count = element_memory->size / sizeof(Node_sll<int>) - node_count;
        nodes = (Node_sll<int> *)in_memory->base;
        edges = nodes + node_count;
    }

    void InitList(int *node_data, int *edge_counts, int *edge_data){
        Stack< Node_sll<int> >stack_ops;
        void *free = nodes;
        for(int i = 0; i <node_count; ++i){
            free = stack_ops.Push(free, Node_sll<int>(node_data[i]));
        }

        int edges_visited = 0;
        for(int i = 0; i < node_count; ++i){
            int node_index = i;
            int edge_count = edge_counts[i];
            InitEdges(node_index, edge_data + edges_visited, edges_visited, edge_count);
            edges_visited+=edge_count;
        }
    }

    void Print(void (*print_fn)(int)){
        SLinkedList<int> linked_list_ops;
        printf("Printing Adjacency List:\n");
        for(int i = 0; i < node_count; ++i){
            printf("\tNode %d edges:\n\t\t", nodes[i].data);
            linked_list_ops.PrintContents(nodes+i, print_fn);
        }
    }
};