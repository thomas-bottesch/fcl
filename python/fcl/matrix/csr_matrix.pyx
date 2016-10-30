import cython
import array
from libc.stdlib cimport malloc, free

cdef class _csr_matrix:
  
  def __cinit__(self):
    self.mtrx = NULL
  
  def to_numpy(self):
    try:
      import scipy
      import scipy.sparse
      import numpy as np
    except:
      raise Exception("conversion of csr matrix to numpy/scipy was requested but unable to import scipy")
    
    cdef POINTER_TYPE number_keys;
    
    number_keys = self.mtrx.pointers[self.mtrx.sample_count]
    keys = np.zeros(number_keys, dtype=np.int64)
    values = np.zeros(number_keys, dtype=np.float64)
    pointers = np.zeros(self.mtrx.sample_count + 1, dtype=np.int64)
    
    for i in xrange(number_keys):
      keys[i] = self.mtrx.keys[i]
      values[i] = self.mtrx.values[i]
    
    for i in xrange(self.mtrx.sample_count + 1):
      pointers[i] = self.mtrx.pointers[i]
    
    return scipy.sparse.csr.csr_matrix((values, keys, pointers), shape=self.shape, copy=False)
    
  def __getitem__(self, i):
    if type(i) != int:
      raise Exception("invalid index %s"%str(i))
    
    if i >= self.mtrx.sample_count:
      raise IndexError("Index is out of range. Index was %d, matrix size is %d"%(i, self.mtrx.sample_count))
    
    keys = array.array('I')
    values = array.array('d')
    
    nnz = self.mtrx.pointers[i + 1] - self.mtrx.pointers[i]
    
    for k in xrange(nnz):
      keys.append(self.mtrx.keys[self.mtrx.pointers[i] + k])
      values.append(self.mtrx.values[self.mtrx.pointers[i] + k])
    
    return keys, values
  
  @property
  def shape(self):
    return (self.mtrx.sample_count, self.mtrx.dim)
  
  def __dealloc__(self):
    if self.mtrx is not NULL:
      free_csr_matrix(self.mtrx);
      free(self.mtrx);
      
cdef _csr_matrix convert_matrix_to_csr_matrix(obj, uint32_t* is_numpy):
  cdef csr_matrix *mtrx
  is_numpy[0] = 0
  
  if type(obj) == _csr_matrix:
    return obj
  
  if type(obj) == str or type(obj) == bytes:
    if type(obj) != bytes:
        obj = str.encode(obj)
        
    res = convert_libsvm_file_to_csr_matrix_wo_labels(obj, &mtrx)
    if (res != 0):
      raise Exception("error while loading input data (possible reason: not correct libsvm format or file does not exist)")

  else:
    try:
      import numpy as np
      import scipy
      import scipy.sparse
      imp_success = True
    except:
      imp_success = False
    
    if imp_success:
      if type(obj) == np.ndarray:
        obj = scipy.sparse.csr.csr_matrix(obj)
      
      if type(obj) == scipy.sparse.csr.csr_matrix:
        keys_array = obj.indices
        values_array = obj.data
        pointer_array = obj.indptr
        no_samples, dim = obj.shape
        mtrx = <csr_matrix *>malloc(cython.sizeof(csr_matrix))
        mtrx.keys =<KEY_TYPE *>malloc(cython.sizeof(KEY_TYPE) * len(keys_array))
        mtrx.values =<VALUE_TYPE *>malloc(cython.sizeof(VALUE_TYPE) * len(values_array))
        mtrx.pointers =<POINTER_TYPE *>malloc(cython.sizeof(POINTER_TYPE) * len(pointer_array))
        mtrx.sample_count = no_samples
        mtrx.dim = dim
        
        for i in xrange(len(keys_array)):
          mtrx.keys[i] = keys_array[i]
          mtrx.values[i] = values_array[i]
        for i in xrange(len(pointer_array)):
          mtrx.pointers[i] = pointer_array[i]
        
        is_numpy[0] = 1
      else:
        raise Exception("cannot convert unknown type (%s) to internal matrix"%type(obj).__name__)
    else:
      raise Exception("cannot convert unknown type (%s) to internal matrix (unable to import numpy and scipy)"%type(obj).__name__)
    
  x = _csr_matrix()
  x.mtrx = mtrx
  return x
      
def get_csr_matrix_from_object(obj):
  cdef uint32_t is_numpy
  return convert_matrix_to_csr_matrix(obj, &is_numpy)

def csr_matrix_to_libsvm_string(obj, labels = None, static_label = 1):
    res = store_matrix_with_label_as_string((<_csr_matrix?> obj).mtrx, NULL, static_label)
    if res == NULL:
      raise Exception("error while converting matrix to string!")
    return res