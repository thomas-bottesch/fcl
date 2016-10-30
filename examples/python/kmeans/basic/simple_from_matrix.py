from __future__ import print_function
from fcl import kmeans
import numpy as np
from scipy import sparse

if __name__ == "__main__":
    # Create random dataset
    np.random.seed(1) # set a random seed to always generate the same matrix X
    X = np.random.rand(100, 100)
    
    # Make matrix sparse by just removing values below 0.5
    X[X < 0.5] = 0
    sparse_X = sparse.csr_matrix(X)
    
    # this example shows how to cluster a numpy matrix.
    km = kmeans.KMeans(no_clusters=10, seed = 0)
    km.fit(sparse_X)
    
    # If the dataset has M samples, then idx is an Mx1 array assigning each sample the closest cluster index.   
    idx = km.predict(sparse_X)
    C = km.get_cluster_centers()   
    
    no_clusters, dim = C.shape
    
    # Get matrices of samples which are in the same cluster
    matrices = {}
    
    for i in range(no_clusters):
        matrices[i] = sparse_X[idx == i]
        print("Cluster %d has shape"%i, matrices[i].shape)
