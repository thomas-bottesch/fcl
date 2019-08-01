from __future__ import print_function
import fcl
import os
import time
from os.path import abspath, join, dirname, isfile
from fcl import kmeans
from fcl.datasets import load_sector_dataset, load_usps_dataset
from fcl.matrix.csr_matrix import get_csr_matrix_from_object, csr_matrix_to_libsvm_string
from sklearn.decomposition import TruncatedSVD, PCA
from scipy.sparse import csr_matrix
from sklearn.datasets import dump_svmlight_file
import numpy as np
import argparse

def get_pca_projection_csrmatrix(fcl_csr_input_matrix, component_ratio):
  n_components = int(fcl_csr_input_matrix.annz * component_ratio)
  p = TruncatedSVD(n_components = n_components)
  start = time.time()
  p.fit(fcl_csr_input_matrix.to_numpy())
  # convert to millis
  fin = (time.time() - start) * 1000 
  (n_samples, n_dim) = fcl_csr_input_matrix.shape
  print("Truncated SVD took %.3fs to retrieve %s components for input_matrix with n_samples %d, n_dim %d" % (fin/1000.0, str(n_components), n_samples, n_dim)) 
  return get_csr_matrix_from_object(p.components_)
  

if __name__ == "__main__":
  parser = argparse.ArgumentParser(description='Create a pca matrix from an input matrix with given component ratio.')
  parser.add_argument('path_input_dataset', type=str, help='Path to the input libsvm dataset')
  parser.add_argument('path_output_dataset', type=str, help='Path to the input libsvm dataset')
  parser.add_argument('--component_ratio', default=0.1, type=float, help='Percentage of the average non zero values of the input dataset to use as components.')
  
  args = parser.parse_args()
  
  if not isfile(args.path_input_dataset):
    raise Exception("Unable to find path_input_dataset: %s" % args.path_input_dataset)
  
  print("Loading data from %s" % args.path_input_dataset)
  fcl_mtrx_input_dataset = get_csr_matrix_from_object(args.path_input_dataset)
  
  print("Retrieving the pca projection matrix")
  pca_mtrx = get_pca_projection_csrmatrix(fcl_mtrx_input_dataset, args.component_ratio)
  
  print("Convert pca projection matrix to libsvm string")
  pca_mtrx_lsvm_str = csr_matrix_to_libsvm_string(pca_mtrx)
  
  print("Writing pca projection matrix libsvm string to file %s" % args.path_output_dataset)
  with open(args.path_output_dataset, 'w') as f:
    f.write(pca_mtrx_lsvm_str)
