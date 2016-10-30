from __future__ import print_function
from ._kmeans import KMEANS_ALGO_INFO, KMEANS_INIT_INFO
from ._kmeans import _kmeans_c, get_csr_matrix_from_object, store_csr_matrix_to_file, StopException
from fcl.cython.utils.matrix.csr_matrix.csr_assign import _assign_c

from fcl.exceptions import StopException
import os
import signal
import random

def is_numpy_available():
  try:
    import numpy
    import scipy
    return True
  except:
    return False

class KMeans():   
    
    def __init__(self, algorithm = "kmeans_optimized", no_clusters = 10
                 , seed = random.randint(0, 2**31), iteration_limit = 1000, tol = 1e-6, n_jobs = -1
                 , remove_empty_clusters = False, verbose = False, result_type = 'auto', init="random"
                 , additional_params = {}, additional_info = {}
                 , create_signal_handler = True):
        
        if n_jobs <= 0:
          n_jobs = -1
        
        if result_type == 'auto':
          self.results_as_numpy = is_numpy_available()
        elif result_type == 'numpy':
          if not is_numpy_available():
            raise Exception("it was requested to return results in numpy format but unable to import numpy!")
          self.results_as_numpy = True
        elif result_type == 'raw':
          self.results_as_numpy = False
        else:
          raise Exception("unknown result_type")
        
        if remove_empty_clusters:
          self.remove_empty_clusters = True
        else:
          self.remove_empty_clusters = False
          
        try:
          tol = float(tol)
        except:
          raise Exception("unable to convert tol to float!")

        if tol <= 0:
          raise Exception("tol must be > 0")
        
        if type(additional_params) != dict:
          raise Exception("additional_params must be a dict!")

        for k, v in additional_params.items():
          if type(k) != str:
            raise Exception("keys of additional_params must be strings!")
        
          if not (type(v) == int or type(v) == float):
            raise Exception("values of additional_params must be either int or float!")
          
          # convert everything to float here
          additional_params[k] = float(additional_params[k])
        
        if type(additional_info) != dict:
          raise Exception("additional_info must be a dict!")

        for k, v in additional_info.items():
          if type(k) != str:
            raise Exception("keys of additional_info must be strings!")
          
          # convert everything to string here
          additional_info[k] = str(additional_info[k])
        
        if no_clusters < 1:
            raise Exception("no_clusters must at least be 1")
        
        if iteration_limit < 1:
            raise Exception("iteration_limit must at least be 1")
        
        if not algorithm in KMEANS_ALGO_INFO:
            raise Exception("unknown algorithm %s"%algorithm)

        if not init in KMEANS_INIT_INFO:
            raise Exception("unknown init %s"%init)
        
        self.additional_params = additional_params
        self.additional_info = additional_info
        
        kmeans_algorithm_id, _ = KMEANS_ALGO_INFO[algorithm]
        init_id, _ = KMEANS_INIT_INFO[init]
               
        self.kmeans_c_obj = _kmeans_c(kmeans_algorithm_id, no_clusters, seed, iteration_limit, tol, n_jobs, init_id, self.remove_empty_clusters, verbose)
        self.cluster_centers_ = None
        self.assign_c_obj = None
        
        if create_signal_handler:
          def handler(signum, frame):
            self.stop()
          
          signal.signal(signal.SIGTERM, handler)
          signal.signal(signal.SIGINT, handler)
          
    def stop(self):
      self.kmeans_c_obj.stop()
      try:
        self.assign_c_obj.stop()
      except:
        pass
    
    def get_tracked_params(self):
      return self.kmeans_c_obj.get_tracked_params()
    
    def fit(self, X):
        self.assign_c_obj = None
        self.cluster_centers_ = self.kmeans_c_obj.fit(X, self.additional_params, self.additional_info)
    
    def fit_predict(self, X, output_distance=False, output_numpy=None):
        self.fit(X)
        return self.predict(X, output_distance, output_numpy)
    
    def _retrieve_numpy(self, output_numpy):
      if output_numpy is None:
        return self.results_as_numpy
      elif output_numpy == True:
        return True
      else:
        return False
    
    def get_cluster_centers(self, output_numpy=None):
      if not self._retrieve_numpy(output_numpy):
        return self.cluster_centers_
      else:
        return self.cluster_centers_.to_numpy()
    
    def predict(self, X, output_distance=False, output_numpy=None):
      if self.cluster_centers_ is None:
        raise Exception("need to fit before predict!")
      
      if self.assign_c_obj is None:
        self.assign_c_obj = _assign_c(self.cluster_centers_)
           
      assignments, distances \
              = self.assign_c_obj.assign_matrix(X, result_as_numpy=self._retrieve_numpy(output_numpy))
      
      if not output_distance:
        return assignments
      else:
        return assignments, distances
    
    def predict_sample(self, keys, values, output_distance=False):
      if self.cluster_centers_ is None:
        raise Exception("need to fit before predict!")
      
      if self.assign_c_obj is None:
        self.assign_c_obj = _assign_c(self.cluster_centers_)
      
      closest_clust, closest_dist = self.assign_c_obj.assign_sparse_vector(keys, values)
      
      if not output_distance:
        return closest_clust
      else:
        return closest_clust, closest_dist
      
    def file_fit(self, path_input_matrix, path_output_clusters):
        if not os.path.isfile(path_input_matrix):
            raise Exception("file %s does not exist"%path_input_matrix)
        
        self.kmeans_c_obj.file_fit(path_input_matrix, path_output_clusters, self.additional_params, self.additional_info)
    
    def to_pickle(self, output_path):
      if self.cluster_centers_ is None:
        raise Exception("need to fit before being able to pickle!")
      
      store_csr_matrix_to_file(self.cluster_centers_, output_path)
      
    def to_pickled_string(self):
      if self.cluster_centers_ is None:
        raise Exception("need to fit before being able to pickle!")
      
      return str(self.cluster_centers_)

def print_algorithm_info():
    print("Available algorithms:")
    sorted_algorithms = sorted([(KMEANS_ALGO_INFO[name], name) for name in KMEANS_ALGO_INFO])
    for info, name in sorted_algorithms:
        print("   %s"%name.ljust(30), info[1])
        
def print_initialization_info():
    print("Available initialization strategies:")
    sorted_initializations = sorted([(KMEANS_INIT_INFO[name], name) for name in KMEANS_INIT_INFO])
    for info, name in sorted_initializations:
        print("   %s"%name.ljust(30), info[1])

def from_pickle(obj_str):
  km = KMeans()
  km.cluster_centers_ = get_csr_matrix_from_object(obj_str)
  return km
  
  
  
        