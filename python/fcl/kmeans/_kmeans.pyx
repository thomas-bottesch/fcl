from __future__ import print_function
import cython
import array
from libc.stdlib cimport malloc, free
from libc.stdint cimport uint32_t, uint64_t, int32_t, UINT32_MAX
from cpython cimport Py_INCREF
from fcl.cython.utils.types cimport KEY_TYPE, POINTER_TYPE, VALUE_TYPE
from fcl.matrix.csr_matrix cimport csr_matrix, _csr_matrix, free_csr_matrix, convert_libsvm_file_to_csr_matrix_wo_labels
from fcl.matrix.csr_matrix cimport store_matrix_with_label, store_matrix_with_label_as_string, convert_matrix_to_csr_matrix
from fcl.cython.utils.cdict cimport cdict, free_cdict, d_add_subfloat, d_add_substring, cdict_to_python_dict
from fcl.matrix.csr_matrix import get_csr_matrix_from_object, csr_matrix_to_libsvm_string
from fcl.cython.utils.matrix.csr_matrix.csr_assign cimport _assign_c
from fcl.exceptions import StopException
from fcl.cython.utils.global_defs cimport omp_set_num_threads

include "../cython/utils/signals.pxi"

cdef extern from "algorithms/kmeans/kmeans_control.h":
    cdef uint32_t NO_KMEANS_ALGOS
    cdef const char **KMEANS_ALGORITHM_NAMES
    cdef const char **KMEANS_ALGORITHM_DESCRIPTION

    cdef uint32_t NO_KMEANS_INITS
    cdef const char **KMEANS_INIT_NAMES
    cdef const char **KMEANS_INIT_DESCRIPTION
       
    ctypedef csr_matrix* (*kmeans_algorithm_function) (csr_matrix* samples, kmeans_params *prms) nogil
    cdef kmeans_algorithm_function* KMEANS_ALGORITHM_FUNCTIONS;

    cdef struct kmeans_params:
      uint32_t kmeans_algorithm_id
      uint32_t no_clusters
      uint32_t seed
      uint32_t iteration_limit
      VALUE_TYPE tol
      uint32_t verbose
      uint32_t init_id
      uint32_t remove_empty
      uint32_t stop
      cdict* tr

cdef get_kmeans_algo_info():
    info = {}
    
    for i in range(NO_KMEANS_ALGOS):
        if type(KMEANS_ALGORITHM_NAMES[i]) != str:
            info[KMEANS_ALGORITHM_NAMES[i].decode()] = (i, KMEANS_ALGORITHM_DESCRIPTION[i].decode())
        else:
            info[KMEANS_ALGORITHM_NAMES[i]] = (i, KMEANS_ALGORITHM_DESCRIPTION[i])
    
    return info

cdef get_kmeans_init_info():
    info = {}
    
    for i in range(NO_KMEANS_INITS):
        if type(KMEANS_INIT_NAMES[i]) != str:
            info[KMEANS_INIT_NAMES[i].decode()] = (i, KMEANS_INIT_DESCRIPTION[i].decode())
        else:
            info[KMEANS_INIT_NAMES[i]] = (i, KMEANS_INIT_DESCRIPTION[i])
            
    return info

KMEANS_ALGO_INFO = get_kmeans_algo_info()
KMEANS_INIT_INFO = get_kmeans_init_info()

cdef write_clusters_to_file(_csr_matrix clusters, char* output_clusters_path, uint32_t static_label):
  res = store_matrix_with_label(clusters.mtrx, NULL, static_label, output_clusters_path)
  if (res != 0):
      raise Exception("Error while writing clusters to %s"%output_clusters_path)

def store_csr_matrix_to_file(obj, output_path, static_label = 1):
  write_clusters_to_file(obj, output_path, static_label)

cdef class _kmeans_c:
    cdef kmeans_params *params
    cluster_centers_ = None
    
    def __cinit__(self
                  , uint32_t kmeans_algorithm_id
                  , uint32_t no_clusters
                  , uint32_t seed
                  , uint32_t iteration_limit
                  , VALUE_TYPE tol
                  , int32_t n_jobs
                  , uint32_t init_id
                  , uint32_t remove_empty
                  , uint32_t verbose):
        
        if (n_jobs > 0):
          omp_set_num_threads(n_jobs)
        
        self.params = <kmeans_params *>malloc(cython.sizeof(kmeans_params))
        
        if self.params is NULL:
            raise MemoryError()
    
        self.params.kmeans_algorithm_id = kmeans_algorithm_id
        self.params.no_clusters = no_clusters
        self.params.seed = seed
        self.params.iteration_limit = iteration_limit
        self.params.tol = tol
        self.params.verbose = verbose
        self.params.init_id = init_id
        self.params.remove_empty = remove_empty
        self.params.stop = 0
        self.params.tr = NULL
    
    cdef _csr_matrix _fit_csr_matrix(self, _csr_matrix input_data):
      cdef csr_matrix *clusters     
      clusters = NULL

      with nogil:         
        clusters = KMEANS_ALGORITHM_FUNCTIONS[self.params.kmeans_algorithm_id](input_data.mtrx, self.params)
      
      wrapped_clusters = _csr_matrix()
      wrapped_clusters.mtrx = clusters
      
      return wrapped_clusters      

    def stop(self):
      self.params.stop = 1

    def reset_params(self, additional_params, additional_info):
        
        self.params.stop = 0
        free_cdict(&(self.params.tr))
        
        for param_name, param_value in additional_params.items():
          if str(param_name) != bytes:
              param_name = str.encode(param_name)
              
          d_add_subfloat(&(self.params.tr)
             , b"additional_params"
             , param_name
             , param_value)
          
        for param_name, param_value in additional_info.items():
          if str(param_name) != bytes:
              param_name = str.encode(param_name)
          
          if str(param_value) != bytes:
              param_value = str.encode(param_value)
              
          d_add_substring(&(self.params.tr)
             , b"info"
             , param_name
             , param_value)

    def fit(self, X, additional_params, additional_info):
        cdef uint32_t is_numpy
        
        self.reset_params(additional_params, additional_info)
        _X = convert_matrix_to_csr_matrix(X, &is_numpy)
        cluster_centers = self._fit_csr_matrix(_X)
        
        if self.params.stop:
          raise StopException("Stop was requested!")
        
        return cluster_centers     
    
    def get_tracked_params(self):
      return cdict_to_python_dict(self.params.tr)
    
    def file_fit(self, input_matrix_path, output_clusters_path, additional_params, additional_info, static_label = 1):
        cdef uint32_t is_numpy

        if type(output_clusters_path) != bytes:
            output_clusters_path = str.encode(output_clusters_path)

        self.reset_params(additional_params, additional_info)
        X = convert_matrix_to_csr_matrix(input_matrix_path, &is_numpy)
        clusters = self._fit_csr_matrix(X)
        write_clusters_to_file(clusters, output_clusters_path, static_label)
       
    def print_params(self):
        print(self.params.kmeans_algorithm_id)
        print(self.params.no_clusters)
        print(self.params.seed)
        print(self.params.iteration_limit)
        print(self.params.verbose)

    def __dealloc__(self):
        free_cdict(&(self.params.tr))
        free(self.params)