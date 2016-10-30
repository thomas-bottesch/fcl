from libc.stdint cimport uint32_t, uint64_t
from fcl.cython.utils.types cimport KEY_TYPE, POINTER_TYPE, VALUE_TYPE
from fcl.matrix.csr_matrix cimport csr_matrix

cdef extern from "<utils/matrix/csr_matrix/csr_assign.h>":
    cdef struct assign_result:
      uint64_t *counts
      uint64_t *assignments
      VALUE_TYPE *distances
      uint64_t len_counts
      uint64_t len_assignments

    assign_result assign(csr_matrix* samples, csr_matrix* clusters, uint32_t* stop) nogil
    void free_assign_result(assign_result* res)
    void assign_vector(KEY_TYPE *input_keys
                       , VALUE_TYPE *input_values
                       , uint64_t input_non_zero_count_vector
                       , csr_matrix* clusters
                       , VALUE_TYPE *vector_lengths_clusters
                       , uint64_t* closest_cluster
                       , VALUE_TYPE* closest_cluster_distance) nogil
                       
cdef extern from "<utils/matrix/csr_matrix/csr_math.h>":
    void calculate_matrix_vector_lengths(csr_matrix *mtrx, VALUE_TYPE** vector_lengths) nogil
    
cdef class _assign_c:
  cdef csr_matrix *mtrx
  cdef VALUE_TYPE* vector_lengths
  cdef uint32_t stop_immediately