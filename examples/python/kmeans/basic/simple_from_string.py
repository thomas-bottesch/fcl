from __future__ import print_function
from fcl import kmeans

if __name__ == "__main__":
    # Create dataset as string
    X = ( '1 1:0.50 2:0.34\n'
        + '1 1:0.13 2:0.11\n'
        + '1 1:0.24 2:0.15\n'
        + '1 1:0.67 2:0.24\n'
        + '1 1:0.12 2:0.89\n'
        + '1 1:0.52       \n'
        + '1 1:0.21 2:0.97\n' )
    
    # this example shows how to cluster a matrix read from a string.
    # the same way a file can be read in libsvm format and passed as well.
    km = kmeans.KMeans(no_clusters=2, seed = 0)
    idx = km.fit_predict(X)

    # Determine which samples fall into the same clusters
    X_splits = X.split("\n")
    clusters = {}
    for sample_id in range(len(idx)):
        cluster_id = idx[sample_id]
        if cluster_id not in clusters:
            clusters[cluster_id] = []
            
        clusters[cluster_id].append(X_splits[sample_id].rstrip())
   
    #Show which samples are in the same cluster
    for cluster_index in clusters:
        print("Samples of cluster %d:"%cluster_index, clusters[cluster_index])