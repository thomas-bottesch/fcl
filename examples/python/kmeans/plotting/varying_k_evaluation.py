from __future__ import print_function
import fcl
import matplotlib
import os
import matplotlib.pyplot as plt
from fcl import kmeans
from fcl.datasets import load_sector_dataset, load_usps_dataset
from os.path import abspath, join, dirname

def do_evaluations(datasets):
  
  dataset_results = {}
  for dataset_name, dataset_as_string in datasets:
    print("Doing evaluations for dataset %s"%dataset_name)
    dataset_results[dataset_name] = {'varying_k': {'avoided_calculations': []}}
    k_values = list(range(1, 102, 20)) + list(range(200, 1001, 200))
    bv_annz = 0.3
    algorithm = "kmeans_optimized"
    
    for k in k_values:
      print("Executing %s with k=%d (bv_annz: %f)"%(algorithm, k, bv_annz))
      km = kmeans.KMeans(n_jobs=1, no_clusters=k, algorithm=algorithm, init='random', additional_params = {'bv_annz': bv_annz}, seed = 0, verbose = False)
      km.fit(dataset_as_string)
      dataset_results[dataset_name]['varying_k']['avoided_calculations'].append(sum(km.get_tracked_params()['iteration_bv_calcs_success']) * 100
                                                                          / (sum(km.get_tracked_params()['iteration_bv_calcs_success'])
                                                                             + sum(km.get_tracked_params()['iteration_full_distance_calcs'])))
  
  for dataset_name in dataset_results:
    plt.plot(k_values,
             dataset_results[dataset_name]['varying_k']['avoided_calculations'], '-', linewidth=3, label = dataset_name)

  plt.legend()
  plt.grid(True)
  
  plt.xlabel('number of clusters')
  plt.ylabel('avoided full distance calculations (percent)')
  plt.title(r'Varying k and observing avoided full distance calculations (bv annz = 0.3)')
  
  destination_filename = join(dirname( __file__ ), "varying_k_evaluation.png")
  plt.savefig(destination_filename)
  
  print("plot was saved in the current folder to: %s"%destination_filename)

if __name__ == "__main__":
  datasets = []
  
  ds_folder = abspath(join(dirname( __file__ ), os.pardir, os.pardir, os.pardir, 'datasets'))
  
  with open(load_sector_dataset(ds_folder), 'r') as f:
    datasets.append(('sector', f.read()))
  
  with open(load_usps_dataset(ds_folder), 'r') as f:
    datasets.append(('usps', f.read()))

  do_evaluations(datasets)
  
  