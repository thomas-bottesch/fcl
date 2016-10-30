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
    dataset_results[dataset_name] = {'searching_best_b': {'durations': [], 'bv_annz': []}}
    
    for i in range(1,101, 10):
      bv_annz = float(i)/100
      print("Executing k-means optimized with bv_annz: %f"%bv_annz)
      km = kmeans.KMeans(n_jobs=1, no_clusters=1000, algorithm="kmeans_optimized", init='random', additional_params = {'bv_annz': bv_annz}, seed = 0, verbose = False)
      km.fit(dataset_as_string)
      dataset_results[dataset_name]['searching_best_b']['durations'].append(km.get_tracked_params()['duration_kmeans'] / 1000)
      dataset_results[dataset_name]['searching_best_b']['bv_annz'].append(km.get_tracked_params()['additional_params']['bv_annz'])
  
  for dataset_name in dataset_results:
    plt.plot(dataset_results[dataset_name]['searching_best_b']['bv_annz'],
             dataset_results[dataset_name]['searching_best_b']['durations'], '-', linewidth=3, label = dataset_name)

  plt.legend()
  plt.grid(True)
  
  plt.xlabel('relative block vector size')
  plt.ylabel('time / s')
  plt.title(r'Varying the block vector size in algorithm kmeans-optimized')
  
  destination_filename = join(dirname( __file__ ), "bv_annz_evaluation.png")
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
  
  