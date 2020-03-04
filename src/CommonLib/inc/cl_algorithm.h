#pragma once

C_HEADER_START

typedef
INT64
(__cdecl FUNC_CompareFunction)(
    _In_        const void                  *A,
    _In_        const void                  *B,
    _In_opt_    void*                       Context
    );

void*
BinarySearch(
    _In_ _In_reads_bytes_(NoOfElements*ElementSize)
                const void                  *Array,
    _In_        DWORD                       NoOfElements,
    _In_        DWORD                       ElementSize,
    _In_        void                        *SearchElement,
    _In_        FUNC_CompareFunction        *CompareFunction,
    _In_opt_    void                        *Context
    );

void*
LinearSearch(
    _In_ _In_reads_bytes_(NoOfElements*ElementSize)
                const void                  *Array,
    _In_        DWORD                       NoOfElements,
    _In_        DWORD                       ElementSize,
    _In_        void                        *SearchElement,
    _In_        FUNC_CompareFunction        *CompareFunction,
    _In_opt_    void                        *Context
    );

C_HEADER_END
