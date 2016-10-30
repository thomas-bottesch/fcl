from __future__ import print_function
import fcl
import os
from fcl import kmeans
from fcl.datasets import load_example_dataset
from os.path import abspath, join, dirname

if __name__ == "__main__":
    # Download dataset and put it into ds_folder
    ds_folder = abspath(join(dirname( __file__ ), os.pardir, os.pardir, os.pardir, 'datasets'))
    dataset_path = load_example_dataset(ds_folder)
    
    # this example shows how to cluster a dataset in libsvm format available under dataset_path.
    km = kmeans.KMeans(no_clusters=10, seed = 0)
    km.fit(dataset_path)
    
    # it is now also possible to directly predict a dataset on the filesystem
    # If the dataset has M samples, then idx is an Mx1 array assigning each sample the closest cluster index.   
    idx = km.predict(dataset_path)
    
    # Determine which samples fall into the same clusters
    clusters = {}
    for sample_id in range(len(idx)):
        cluster_id = idx[sample_id]
        if cluster_id not in clusters:
            clusters[cluster_id] = []
            
        clusters[cluster_id].append(sample_id)
   
    #Show which samples are in the same cluster
    for cluster_index in clusters:
        print("Samples of cluster %d:"%cluster_index, clusters[cluster_index])
  