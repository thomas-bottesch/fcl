from __future__ import print_function
import fcl
import os
import matplotlib
import matplotlib.pyplot as plt
import time
from os.path import abspath, join, dirname
from fcl import kmeans
from fcl.datasets import load_sector_dataset, load_usps_dataset
from fcl.matrix.csr_matrix import get_csr_matrix_from_object, csr_matrix_to_libsvm_string
from sklearn.decomposition import TruncatedSVD, PCA
from scipy.sparse import csr_matrix
from sklearn.datasets import dump_svmlight_file
import numpy as np

def do_evaluations(dataset_path, dataset_name):
  
  # Load dataset directly into csr matrix format this way it only needs to be converted once
  data_as_csrmatrix = get_csr_matrix_from_object(dataset_path)
  
  print("Doing evaluations for dataset %s"%dataset_path)
  
  algorithm_results = {'bv_kmeans': {}
                       , 'yinyang': {}
                       , 'bv_yinyang': {}
                       , 'elkan': {}
                       , 'pca_kmeans': {}
                       , 'pca_elkan': {}
                       , 'pca_yinyang': {}
                       }
  
  additional_algo_data =  {}
  
  calculate_svd = False
  for algo in algorithm_results:
    if "pca" in algo:
      calculate_svd = True
      break
  
  if calculate_svd:
    p = TruncatedSVD(n_components = int(data_as_csrmatrix.annz * 0.1))
    start = time.time()
    p.fit(data_as_csrmatrix.to_numpy())
    # convert to millis
    fin = (time.time() - start) * 1000 
    print(fin) 
    pca_projection_csrmatrix = get_csr_matrix_from_object(p.components_)
    for algorithm in algorithm_results:
      if algorithm.startswith("pca_"):
        additional_algo_data[algorithm] = {'data': pca_projection_csrmatrix, 'duration': fin}
    
  clusters = [100, 250, 1000]
  
  # Do the evaluations for every algorithm and every no_clusters
  for algorithm in sorted(algorithm_results.keys()):
    for no_clusters in clusters:
      print("Executing k-means with algorithm: %s and k=%d"%(algorithm, no_clusters))
      km = kmeans.KMeans(n_jobs=1, no_clusters=no_clusters, algorithm=algorithm, init='random', seed = 0, verbose = True)
      if algorithm not in additional_algo_data:
        km.fit(data_as_csrmatrix)
      else:
        km.fit(data_as_csrmatrix, external_vectors = additional_algo_data[algorithm]['data'])
        
      algorithm_results[algorithm][no_clusters] = km.get_tracked_params()
      if algorithm in additional_algo_data:
        algorithm_results[algorithm][no_clusters]['duration_kmeans'] += additional_algo_data[algorithm]['duration']
    
  # plot the results
  plot_overall_duration(algorithm_results, dataset_name)

def plot_overall_duration(algorithm_results, dataset_name):
  # create subplot showing the overall duration of the algorithm

  fig = plt.figure()
  ax = plt.subplot(111)

  i = 0
  tick_pos = []
  sorted_algorithms = sorted(algorithm_results.keys())
  no_clusters_list = sorted(algorithm_results[sorted_algorithms[0]])
  width = 1 / float(len(algorithm_results) + 1)
  base_tick_pos = (float(width) * len(algorithm_results)) / 2
  tick_pos = []
  
  algorithm_xvalues = {}
  algorithm_yvalues = {}
  algorithm_legend_data = {}
  for i in range(len(sorted_algorithms)):
    algo_color = next(ax._get_lines.prop_cycler)['color']
    algo_name = sorted_algorithms[i]
    algorithm_legend_data[i] = (algo_color, algo_name)
    algorithm_xvalues[i] = []
    algorithm_yvalues[i] = []
  
  j = 0
  for no_clusters in no_clusters_list:
    i = 0
    for algorithm in sorted_algorithms:
      algorithm_xvalues[i].append(j + (i * width))
      algorithm_yvalues[i].append(algorithm_results[algorithm][no_clusters]['duration_kmeans'] / 1000)
      i += 1
      
    tick_pos.append(j + base_tick_pos)
    j += 1

  legend_rects = []
  for i in range(len(sorted_algorithms)):
    (algo_color, algo_name) = algorithm_legend_data[i]
    r = ax.bar(algorithm_xvalues[i], algorithm_yvalues[i], width, color=algo_color)
    legend_rects.append(r[0])

  # Shrink current axis by 20%
  box = ax.get_position()
  ax.set_position([box.x0, box.y0, box.width * 0.8, box.height])

  ax.set_xticks(tick_pos, minor=False)
  ax.set_xticklabels(no_clusters_list, fontdict=None, minor=False)
  ax.set_ylabel('time / s')
  ax.set_xlabel('number of clusters')
  ax.set_title("Overall duration of various algorithms for dataset %s"%dataset_name)

  lgd = ax.legend(legend_rects, sorted_algorithms, loc='center left', bbox_to_anchor=(1, 0.5))

  fig.tight_layout()
  destination_filename = join(dirname( __file__ ), "algorithm_speed_comparison.png")
  plt.savefig(destination_filename, bbox_extra_artists=(lgd,), bbox_inches='tight')
  print("plot was saved in the current folder to: %s"%destination_filename)

if __name__ == "__main__":
  ds_folder = abspath(join(dirname( __file__ ), os.pardir, os.pardir, os.pardir, 'datasets'))
  do_evaluations(load_sector_dataset(ds_folder), 'sector')