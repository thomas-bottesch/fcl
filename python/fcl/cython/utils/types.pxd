from libc.stdint cimport uint32_t, uint64_t, int32_t

cdef extern from "<utils/types.h>":
    ctypedef uint32_t KEY_TYPE
    ctypedef uint64_t POINTER_TYPE
    ctypedef double VALUE_TYPE