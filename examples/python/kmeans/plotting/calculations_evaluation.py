from __future__ import print_function
import fcl
import os
import matplotlib
import matplotlib.pyplot as plt
from os.path import abspath, join, dirname
from fcl import kmeans
from fcl.datasets import load_sector_dataset, load_usps_dataset


def plot_overall_duration_subplot(ax, algorithm_results):
  # create subplot showing the overall duration of the algorithm

  i = 0
  tick_pos = []
  for algorithm in algorithm_results:
    ax.bar(i, algorithm_results[algorithm]['duration_kmeans'] / 1000)
    tick_pos.append(i + 0.5)
    i += 1

  ax.set_xticks(tick_pos, minor=False)
  ax.set_xticklabels(algorithm_results.keys(), fontdict=None, minor=False, rotation=45)
  ax.set_ylabel('time / s')
  ax.set_title("Overall duration")

def plot_fulldist_calcs_subplot(ax, algorithm_results):
  # create subplot showing the number of full distance calculations for every iteration
  for algorithm in algorithm_results:
    full_distance_calcs = algorithm_results[algorithm]['iteration_full_distance_calcs']
    
    if 'iteration_bv_calcs' in algorithm_results[algorithm]:
      for i in range(algorithm_results[algorithm]['no_iterations']):
        # block vector calculations also have certain costs
        # these costs are added here to the full distance calculations
        full_distance_calcs[i] += algorithm_results[algorithm]['iteration_bv_calcs'][i] \
                                  * algorithm_results[algorithm]['additional_params']['bv_annz']
      
    
    ax.plot(range(algorithm_results[algorithm]['no_iterations']),
             full_distance_calcs, '-', linewidth=3, label = algorithm)

  ax.legend()
  ax.grid(True)
  
  ax.set_xlabel('iteration')
  ax.set_ylabel('full distance calculations')
  ax.set_title("Full distance calculations per iteration")
  
def plot_iteration_duration_subplot(ax, algorithm_results):
  # create subplot showing the number of full distance calculations for every iteration
  for algorithm in algorithm_results:
    print(algorithm
          , algorithm_results[algorithm]['iteration_durations']
          , sum(algorithm_results[algorithm]['iteration_durations'])
          , len(algorithm_results[algorithm]['iteration_durations'])
          , algorithm_results[algorithm]['no_iterations']
          , list(map(lambda x: x / 1000.0, algorithm_results[algorithm]['iteration_durations'])))
    ax.plot(range(algorithm_results[algorithm]['no_iterations']),
             list(map(lambda x: x / 1000.0, algorithm_results[algorithm]['iteration_durations'])), '-', linewidth=3, label = algorithm)

  ax.legend()
  ax.grid(True)
  
  ax.set_xlabel('iteration')
  ax.set_ylabel('time / s')
  ax.set_title("Duration of every iter")

def do_evaluations(dataset_path): 
  print("Doing evaluations for dataset %s"%dataset_path)
  algorithm_results = {'bv_kmeans': None, 'yinyang': None}
  
  for algorithm in algorithm_results:
    print("Executing k-means with algorithm: %s"%algorithm)
    km = kmeans.KMeans(n_jobs=1, no_clusters=300, algorithm=algorithm, init='random', seed = 0, verbose = False)
    km.fit(dataset_path)
    algorithm_results[algorithm] = km.get_tracked_params()   
  
  f, (a0, a1, a2) = plt.subplots(1,3, gridspec_kw = {'width_ratios':[1, 4, 4]}, figsize=(18, 8))
  
  plot_overall_duration_subplot(a0, algorithm_results)
  plot_iteration_duration_subplot(a1, algorithm_results)
  plot_fulldist_calcs_subplot(a2, algorithm_results)

  f.tight_layout()
  destination_filename = join(dirname( __file__ ), "calculations_evaluation.png")
  plt.savefig(destination_filename)
  
  print("plot was saved in the current folder to: %s"%destination_filename)

if __name__ == "__main__":
  ds_folder = abspath(join(dirname( __file__ ), os.pardir, os.pardir, os.pardir, 'datasets'))
  do_evaluations(load_sector_dataset(ds_folder))
  
  