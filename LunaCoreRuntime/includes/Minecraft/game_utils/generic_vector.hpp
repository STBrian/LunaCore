#pragma once

class GenericVector {
    public:
    void* field1 = nullptr;
    void* field2 = nullptr;
    void* field3 = nullptr;
};

template <typename T>
class GenericVectorType {
    public:
    GenericVector gvector;

    // This includes a destructor
    ~GenericVectorType() {
        for (u32* current = (u32*)gvector.field1; current != (u32*)gvector.field2; current = (u32*)((u32)current + sizeof(T))) {
            reinterpret_cast<T*>(current)->~T();
        }
    }
};