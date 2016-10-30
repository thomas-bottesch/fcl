from __future__ import print_function
from fcl import kmeans

if __name__ == "__main__":
    # the kmeans package provides multiple algorithms which can be specified
    # in the following way:
    # e.g. kmeans.KMeans(algorithm = 'kmeans_optimized')
    
    # To list all possible algorithms use:
    kmeans.print_algorithm_info()
    
    print() # Create emptyline
    
    # Also various initializations are possible by setting
    # e.g. kmeans.KMeans(init = 'kmeans++'
    
    # To list all possible initializations use:
    kmeans.print_initialization_info()