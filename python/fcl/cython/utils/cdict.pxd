from libc.stdint cimport uint32_t, uint64_t
from fcl.cython.utils.types cimport KEY_TYPE, POINTER_TYPE, VALUE_TYPE

cdef extern from "<utils/cdict.h>":

    cdef uint32_t DICT_I       # cdict data type single integer
    cdef uint32_t DICT_F       # cdict data type single float
    cdef uint32_t DICT_D       # cdict data type cdict
    cdef uint32_t DICT_DL      # cdict data type list of cdict
    cdef uint32_t DICT_FL      # cdict data type list of floats
    cdef uint32_t DICT_IL      # cdict data type list of unsigned integers
    cdef uint32_t DICT_ST      # cdict data type is string
    cdef uint32_t DICT_SL      # cdict data type is list of strings

    cdef struct cdict:
        char* name;
        uint32_t type;
        VALUE_TYPE fval;
        uint64_t val;
        uint64_t alloced;
        uint64_t* val_list;
        VALUE_TYPE* fval_list;
        char** sval_list;
        char* sval;
        cdict* d_val;
        cdict** d_list;
        
    void free_cdict(cdict** d);
    void d_add_subfloat(cdict** d, char* subdict_name, char* name, VALUE_TYPE fval);
    void d_add_substring(cdict** d, char* subdict_name, char* name, char* sval);
    cdict* d_next(cdict* element);
    
cdef cdict_to_python_dict(cdict* _d)