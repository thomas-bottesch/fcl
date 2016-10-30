from fcl.cython.utils.types cimport KEY_TYPE, POINTER_TYPE, VALUE_TYPE, uint32_t, uint64_t, int32_t

cdef extern from "<utils/matrix/csr_matrix/csr_matrix.h>":

    cdef struct csr_matrix:
        KEY_TYPE *keys;
        VALUE_TYPE *values;
        POINTER_TYPE *pointers;
        uint64_t sample_count;
        uint64_t dim;
    
    void free_csr_matrix(csr_matrix *mtrx) nogil;

cdef extern from "<utils/matrix/csr_matrix/csr_store_matrix.h>":
    
    uint32_t store_matrix_with_label(csr_matrix *mtrx, int32_t* labels, int32_t static_label, char* output_path) nogil
    char* store_matrix_with_label_as_string(csr_matrix *mtrx, int32_t* labels, int32_t static_label) nogil
    
cdef extern from "<utils/matrix/csr_matrix/csr_load_matrix.h>":    
    uint32_t convert_libsvm_file_to_csr_matrix_wo_labels(const char *filename, csr_matrix **mtrx) nogil

                      
cdef class _csr_matrix:
  cdef csr_matrix *mtrx
  
cdef _csr_matrix convert_matrix_to_csr_matrix(obj, uint32_t* is_numpy)