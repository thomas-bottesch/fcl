from __future__ import print_function
import cython
import array
from libc.stdlib cimport malloc, free, calloc
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



cdef extern from "algorithms/kmeans/init_params.h":
    cdef const char* TOKEN_INITIAL_CLUSTER_SAMPLES
    cdef const char* TOKEN_ASSIGNMENTS

    cdef struct initialization_params:
      uint64_t  len_assignments
      uint64_t* assignments
      uint64_t  len_initial_cluster_samples
      uint64_t* initial_cluster_samples
                                     
    void free_init_params(initialization_params* initprms) nogil
    
cdef extern from "algorithms/kmeans/kmeans_control.h":
    cdef uint32_t NO_KMEANS_ALGOS
    cdef const char **KMEANS_ALGORITHM_NAMES
    cdef const char **KMEANS_ALGORITHM_DESCRIPTION

    cdef uint32_t NO_KMEANS_INITS
    cdef const char **KMEANS_INIT_NAMES
    cdef const char **KMEANS_INIT_DESCRIPTION

    cdef struct kmeans_result:
      csr_matrix* clusters
      initialization_params* initprms
    
    ctypedef kmeans_result* (*kmeans_algorithm_function) (csr_matrix* samples, kmeans_params *prms) nogil
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
      csr_matrix* ext_vects
      initialization_params* initprms

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

cdef get_initialization_params_token():
  return TOKEN_INITIAL_CLUSTER_SAMPLES, TOKEN_ASSIGNMENTS

KMEANS_ALGO_INFO = get_kmeans_algo_info()
KMEANS_INIT_INFO = get_kmeans_init_info()
INIT_PRMS_TOKEN_INITIAL_CLUSTER_SAMPLES, INIT_PRMS_TOKEN_ASSIGNMENTS = get_initialization_params_token()

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
                  , initprms
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
        self.params.ext_vects = NULL
        self.params.initprms = NULL
        
        if initprms is not None:
          if type(initprms) != dict:
            raise Exception("initialization_params must be a dict!")
          required_tokens = [(TOKEN_INITIAL_CLUSTER_SAMPLES, list, int),
                             (TOKEN_ASSIGNMENTS, list, int)]
          for token, list_type, element_type in required_tokens:
            if token not in initprms:
              raise Exception("Required token %s not in initialization_params" % token)
            if type(initprms[token]) != list_type:
              raise Exception("initialization_params[%s] needs to be of type %s" % str(list_type))
            for entry in initprms[token]:
              if type(entry) != element_type:
                raise Exception("elements in initialization_params[%s] need to be of type %s" % str(element_type))
          
          self.params.initprms = <initialization_params *>malloc(cython.sizeof(initialization_params))
          if self.params.initprms is NULL:
              raise MemoryError()
          self.params.initprms.assignments = NULL
          self.params.initprms.initial_cluster_samples = NULL
          self.params.initprms.len_assignments = len(initprms[TOKEN_ASSIGNMENTS])
          self.params.initprms.len_initial_cluster_samples = len(initprms[TOKEN_INITIAL_CLUSTER_SAMPLES])
          
          self.params.initprms.assignments = <uint64_t *>malloc(cython.sizeof(uint64_t) * self.params.initprms.len_assignments)
          if self.params.initprms.assignments is NULL:
              raise MemoryError()
          
          self.params.initprms.initial_cluster_samples = <uint64_t *>malloc(cython.sizeof(uint64_t) * self.params.initprms.len_initial_cluster_samples)
          if self.params.initprms.initial_cluster_samples is NULL:
              raise MemoryError()
            
          for i in range(len(initprms[TOKEN_ASSIGNMENTS])):
            self.params.initprms.assignments[i] = initprms[TOKEN_ASSIGNMENTS][i]
            
          for i in range(len(initprms[TOKEN_INITIAL_CLUSTER_SAMPLES])):
            self.params.initprms.initial_cluster_samples[i] = initprms[TOKEN_INITIAL_CLUSTER_SAMPLES][i]
          
    cdef _fit_csr_matrix(self, _csr_matrix input_data):
      cdef csr_matrix *clusters     
      clusters = NULL

      with nogil:         
        kmeans_result = KMEANS_ALGORITHM_FUNCTIONS[self.params.kmeans_algorithm_id](input_data.mtrx, self.params)
      
      wrapped_clusters = _csr_matrix()
      wrapped_clusters.mtrx = kmeans_result.clusters
      
      python_res = {}
      python_res['clusters'] = wrapped_clusters
      python_res['initialization_params'] = {}
      python_res['initialization_params'][TOKEN_ASSIGNMENTS] = [0] * kmeans_result.initprms.len_assignments
      python_res['initialization_params'][TOKEN_INITIAL_CLUSTER_SAMPLES] = [0] * kmeans_result.initprms.len_initial_cluster_samples
      
      for i in range(kmeans_result.initprms.len_assignments):
        python_res['initialization_params'][TOKEN_ASSIGNMENTS][i] = int(kmeans_result.initprms.assignments[i])

      for i in range(kmeans_result.initprms.len_initial_cluster_samples):
        python_res['initialization_params'][TOKEN_INITIAL_CLUSTER_SAMPLES][i] = int(kmeans_result.initprms.initial_cluster_samples[i])
      
      free_init_params(kmeans_result.initprms)
      free(kmeans_result.initprms)
      free(kmeans_result)
      return python_res      

    def stop(self):
      self.params.stop = 1

    cdef reset_params(self, additional_params, additional_info, _csr_matrix external_vectors):
        cdef uint32_t is_numpy
        self.params.stop = 0
        free_cdict(&(self.params.tr))
        
        if external_vectors is None:
          self.params.ext_vects = NULL
        else:
          self.params.ext_vects =  external_vectors.mtrx     
        
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
          
          return external_vectors

    def fit(self, X, additional_params, additional_info, external_vectors=None):
        cdef uint32_t is_numpy       
        
        if external_vectors is not None:
          external_vectors = convert_matrix_to_csr_matrix(external_vectors, &is_numpy)
          
        self.reset_params(additional_params, additional_info, external_vectors)
        _X = convert_matrix_to_csr_matrix(X, &is_numpy)
        python_res = self._fit_csr_matrix(_X)
        
        if self.params.stop:
          raise StopException("Stop was requested!")
        
        return python_res     
    
    def get_tracked_params(self):
      return cdict_to_python_dict(self.params.tr)
    
    def file_fit(self, input_matrix_path, output_clusters_path, additional_params, additional_info, static_label = 1, external_vectors=None):
        cdef uint32_t is_numpy

        if external_vectors is not None:
          external_vectors = convert_matrix_to_csr_matrix(external_vectors, &is_numpy)
          
        if type(output_clusters_path) != bytes:
            output_clusters_path = str.encode(output_clusters_path)

        self.reset_params(additional_params, additional_info, external_vectors)
        X = convert_matrix_to_csr_matrix(input_matrix_path, &is_numpy)
        python_res = self._fit_csr_matrix(X)
        write_clusters_to_file(python_res['clusters'], output_clusters_path, static_label)
       
    def print_params(self):
        print(self.params.kmeans_algorithm_id)
        print(self.params.no_clusters)
        print(self.params.seed)
        print(self.params.iteration_limit)
        print(self.params.verbose)

    def __dealloc__(self):
      if self.params is not NULL:
        if self.params.initprms is not NULL:
          free_init_params(self.params.initprms)
          free(self.params.initprms)
        free_cdict(&(self.params.tr))
        free(self.params)