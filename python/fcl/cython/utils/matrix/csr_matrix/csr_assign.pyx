from fcl.matrix.csr_matrix cimport _csr_matrix, convert_matrix_to_csr_matrix
from libc.stdlib cimport malloc, free
from fcl.exceptions import StopException
import cython

include "../../signals.pxi"

cdef class _assign_c:
  
  def __cinit__(self, _csr_matrix mtrx_wrapped):
    self.mtrx = mtrx_wrapped.mtrx
    self.stop_immediately = 0   
    calculate_matrix_vector_lengths(self.mtrx, &self.vector_lengths)
  
  def stop(self):
    self.stop_immediately = 1
  
  def assign_matrix(self, mtrx_to_assign, result_as_numpy=True):
    cdef assign_result res
    cdef uint32_t is_numpy
    
    self.stop_immediately = 0
    if type(mtrx_to_assign) != _csr_matrix:
      converted_matrix = convert_matrix_to_csr_matrix(mtrx_to_assign, &is_numpy)
    else:
      converted_matrix = <_csr_matrix?>mtrx_to_assign
    
    res = assign(converted_matrix.mtrx, self.mtrx, &self.stop_immediately)
    
    assignments = []
    distances = []
    for i in xrange(res.len_assignments):
      assignments.append(res.assignments[i])
      distances.append(res.distances[i])
      
    free_assign_result(&res)
    
    if self.stop_immediately:
      raise StopException("Stop was requested!")
    
    if result_as_numpy:
      try:
        import numpy as np
        return np.array(assignments), np.array(distances)
      except:
        raise Exception("requested to get the result as numpy array but numpy cannot be imported!")
    else:
      return assignments, distances
  
  def assign_sparse_vector(self, keys, values):
    cdef uint64_t closest_cluster
    cdef VALUE_TYPE closest_cluster_distance
    cdef KEY_TYPE* ckeys
    cdef VALUE_TYPE* cvalues
    cdef uint64_t nnz
    
    if len(keys) != len(values):
      raise Exception("keys and values must have the same length!")
    
    ckeys = <KEY_TYPE *>malloc(len(keys)*cython.sizeof(KEY_TYPE))
    if ckeys is NULL:
        raise MemoryError()
      
    cvalues = <VALUE_TYPE *>malloc(len(values)*cython.sizeof(VALUE_TYPE))
    if cvalues is NULL:
        raise MemoryError()

    for i in xrange(len(values)):
        ckeys[i] = keys[i]
        cvalues[i] = values[i]
    
    nnz = len(keys)
    assign_vector(ckeys
                 , cvalues
                 , nnz
                 , self.mtrx
                 , self.vector_lengths
                 , &closest_cluster
                 , &closest_cluster_distance)
    
    free(ckeys)
    free(cvalues)
    
    return (closest_cluster, closest_cluster_distance)
  
  def __dealloc__(self):
    free(self.vector_lengths)
