# This example creates a comparison between k-means algorithms implementend in fcl
# and the k-means algorithm implemented in scikit-learn.
# The comparison contains optimized and unoptimized version of k-means.
# All comparisons are done with sparse input data. In scikit-learn only the
# traditional k-means algorithm is available for clustering while more sophisticated
# algorithms like 'elkan' do not work yet with sparse data.

from __future__ import print_function
import fcl
import os
import matplotlib
import matplotlib.pyplot as plt
from os.path import abspath, join, dirname
from fcl import kmeans
from fcl.datasets import load_sector_dataset, load_usps_dataset
from fcl.matrix.csr_matrix import get_csr_matrix_from_object 
import time
import sklearn.cluster

def timeit(km, X):
  start = time.time()
  km.fit(X)
  km.predict(X)
  return time.time() - start

def do_evaluations(dataset_path, dataset_name):
  
  # Load dataset directly into csr matrix format this way it only needs to be converted once
  data_as_csrmatrix = get_csr_matrix_from_object(dataset_path)
  
  # Convert data to numpy array
  data_as_numpy = data_as_csrmatrix.to_numpy()
  
  # sklearn 
  # - uses dense vectors to store the cluster centers. this makes the calculation of the distance between
  #   sparse samples and dense clusters very fast. However if the input data has very large dimensions, storing
  #   dense cluster centers gets very costly.
  #
  # fcl
  # - uses sparse vectors everywhere. Calculating distances between a sparse center and a sparse sample is a lot more expensive
  #   then using a dense center. However it is possible to cluster very high dimensional data into many clusters on a regular
  #   while keeping the memory usage very low.
  algorithm_results = {}
  
  # These values generate the official repository plot.
  #algorithms = ["elkan", "bv_kmeans", "yinyang", "bv_yinyang", "kmeans"]
  #clusters = [10, 50, 100, 500, 1000]
  
  # These values are used in order to allow the tests to be more efficient
  algorithms = ["kmeans"]
  clusters = [2, 8]
  
  for no_clusters in clusters:
    for algorithm in algorithms:
      algorithm_name = "fcl_kmeans_" + algorithm
      if not algorithm_name in algorithm_results:
        algorithm_results[algorithm_name] = {}
      print("evaluating: fcl kmeans (%s) with k=%d and dataset %s"%(algorithm, no_clusters, dataset_name))
      dur = timeit(kmeans.KMeans(n_jobs=1, no_clusters=no_clusters, algorithm=algorithm, init='random', seed = 1, verbose = False)
             , data_as_csrmatrix)
      algorithm_results[algorithm_name][no_clusters] = dur
    
    algorithm_name = "sklearn_kmeans"
    if not algorithm_name in algorithm_results:
      algorithm_results[algorithm_name] = {}
    
    # Evaluating the speed of scikit-learn when clustering a sparse matrix
    print("evaluating: sklearn kmeans (sparse matrix) with k=%d and dataset %s"%(no_clusters, dataset_name))
    dur = timeit(sklearn.cluster.KMeans(n_init = 1, n_jobs=1, n_clusters=no_clusters, algorithm='full', init='random', random_state=1)
               , data_as_numpy)
    algorithm_results[algorithm_name][no_clusters] = dur
    

  # plot the results
  plot_sklearn_comparison(algorithm_results, dataset_name)

def plot_sklearn_comparison(algorithm_results, dataset_name):
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
      algorithm_yvalues[i].append(algorithm_results[algorithm][no_clusters])
      i += 1
      
    tick_pos.append(j + base_tick_pos)
    j += 1

  legend_rects = []
  for i in range(len(sorted_algorithms)):
    (algo_color, algo_name) = algorithm_legend_data[i]
    r = ax.bar(algorithm_xvalues[i], algorithm_yvalues[i], width, color=algo_color)
    legend_rects.append(r[0])

  ax.set_xticks(tick_pos, minor=False)
  ax.set_xticklabels(no_clusters_list, fontdict=None, minor=False)
  ax.set_ylabel('time / s')
  ax.set_xlabel('number of clusters')
  ax.set_title("Overall duration of fcl vs sklearn (kmeans) for dataset %s"%dataset_name)

  lgd = ax.legend(legend_rects, sorted_algorithms, loc='upper left')

  fig.tight_layout()
  destination_filename = join(dirname( __file__ ), "algorithm_speed_comparison_sklearn.png")
  plt.savefig(destination_filename, bbox_extra_artists=(lgd,), bbox_inches='tight')
  print("plot was saved in the current folder to: %s"%destination_filename)

if __name__ == "__main__":
  ds_folder = abspath(join(dirname( __file__ ), os.pardir, os.pardir, os.pardir, 'datasets'))
  do_evaluations(load_sector_dataset(ds_folder), 'sector')