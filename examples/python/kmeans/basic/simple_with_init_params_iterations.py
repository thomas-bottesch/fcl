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
    
    initialization_params = None
    
    # The purpose of this example is to go step by step through the kmeans algorithm and output
    # the assignments of each step
    
    print()
    print("Initialization output parameters:")
    iterations = 0

    while True:    
      # this example shows how to cluster a matrix read from a string.
      # the same way a file can be read in libsvm format and passed as well.
      km = kmeans.KMeans(no_clusters=2, seed=4, iteration_limit=1, initialization_params = initialization_params, verbose=False)
      idx = km.fit_predict(X) 
      initialization_params = km.get_output_initialization_params()
      tracked_params = km.get_tracked_params()
      iterations += 1
      pprint.pprint(initialization_params)
      if tracked_params['iteration_changes'][0] == 0:
        break
    
    print("Algorithm finished after %d iterations cause there were no more changes between the last two assignments" % iterations)