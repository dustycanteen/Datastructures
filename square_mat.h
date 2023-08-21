#pragma once


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