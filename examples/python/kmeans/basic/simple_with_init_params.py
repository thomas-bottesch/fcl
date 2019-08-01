from __future__ import print_function
from fcl import kmeans
import pprint


if __name__ == "__main__":
    # Create dataset as string
    X = ( '1 1:0.50 2:0.34\n'
        + '1 1:0.13 2:0.11\n'
        + '1 1:0.24 2:0.15\n'
        + '1 1:0.67 2:0.24\n'
        + '1 1:0.12 2:0.89\n'
        + '1 1:0.52       \n'
        + '1 1:0.21 2:0.97\n' )
    
    # we create a initialization params dict
    initialization_params = {}
    
    # this specifies which samples shall initially be used as clusters
    initialization_params[kmeans.INIT_PRMS_TOKEN_INITIAL_CLUSTER_SAMPLES] = [3, #'1 1:0.24 2:0.15\n'
                                                                             3,
                                                                             3] #'1 1:0.52       \n'
    
    # this specifies to which sample every cluster should initially be assigned to
    initialization_params[kmeans.INIT_PRMS_TOKEN_ASSIGNMENTS] = [0,
                                                                 1,
                                                                 1, # closest to iself
                                                                 0,
                                                                 0,
                                                                 2, # closest to itself
                                                                 1]
    
    # this example shows how to cluster a matrix read from a string.
    # the same way a file can be read in libsvm format and passed as well.
    km = kmeans.KMeans(no_clusters=2, seed = 1, initialization_params = initialization_params)
    idx = km.fit_predict(X)
    init_params_out = km.get_output_initialization_params()
    
    # output the input initialization params
    print()
    print("Initialization input parameters:")
    pprint.pprint(initialization_params)
    
    # to be able to compare them with the output initialization params
    print()
    print("Initialization output parameters:")
    pprint.pprint(init_params_out)
    
    # Determine which samples fall into the same clusters
    X_splits = X.split("\n")
    clusters = {}
    for sample_id in range(len(idx)):
        cluster_id = idx[sample_id]
        if cluster_id not in clusters:
            clusters[cluster_id] = []
            
        clusters[cluster_id].append(X_splits[sample_id].rstrip())
    print()
    print("Show which samples are in the same cluster")
    #Show which samples are in the same cluster
    for cluster_index in clusters:
        print("Samples of cluster %d:"%cluster_index, clusters[cluster_index])