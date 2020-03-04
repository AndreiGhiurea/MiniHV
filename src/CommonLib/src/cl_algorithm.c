#include "common_lib.h"
#include "cl_algorithm.h"

#define ARRAY_ELEM_AT(Arr,Sz,i)             PtrOffset((Arr), (i) * (Sz))

void*
BinarySearch(
    _In_ _In_reads_bytes_(NoOfElements*ElementSize)
                const void                  *Array,
    _In_        DWORD                       NoOfElements,
    _In_        DWORD                       ElementSize,
    _In_        void                        *SearchElement,
    _In_        FUNC_CompareFunction        *CompareFunction,
    _In_opt_    void                        *Context
    )
{
    if (Array == NULL) return NULL;
    if (NoOfElements == 0) return NULL;
    if (ElementSize == 0) return NULL;
    if (SearchElement == NULL) return NULL;
    if (CompareFunction == NULL) return NULL;

    DWORD left = 0;
    DWORD right = NoOfElements - 1;

    while (left <= right)
    {
        // add 2 DWORDS may result QWORD, divide by 2 => DWORD
        DWORD middle = (DWORD)(((QWORD)left + right) / 2);

        INT64 cmpResult = CompareFunction(
            ARRAY_ELEM_AT(Array, ElementSize, middle),
            SearchElement,
            Context);

        if (cmpResult > 0) right = middle - 1;
        else if (cmpResult < 0) left = middle + 1;
        else return ARRAY_ELEM_AT(Array, ElementSize, middle);
    }

    return NULL;
}

void*
LinearSearch(
    _In_ _In_reads_bytes_(NoOfElements*ElementSize)
                const void                  *Array,
    _In_        DWORD                       NoOfElements,
    _In_        DWORD                       ElementSize,
    _In_        void                        *SearchElement,
    _In_        FUNC_CompareFunction        *CompareFunction,
    _In_opt_    void                        *Context
    )
{
    if (Array == NULL) return NULL;
    if (NoOfElements == 0) return NULL;
    if (ElementSize == 0) return NULL;
    if (SearchElement == NULL) return NULL;
    if (CompareFunction == NULL) return NULL;

    for (DWORD i = 0; i < NoOfElements; ++i)
    {
        if (CompareFunction(ARRAY_ELEM_AT(Array, ElementSize, i), SearchElement, Context) == 0) return ARRAY_ELEM_AT(Array, ElementSize, i);
    }

    return NULL;
}
