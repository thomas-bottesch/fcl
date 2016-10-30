function simple_from_string()
    addpath(fileparts(fileparts(fileparts(mfilename('fullpath')))));
    check_compile('kmeans')
    
    % This example shows how to cluster a libsvm string
    X = sprintf(['1 1:0.50 2:0.34\n'...
                ,'1 1:0.13 2:0.11\n'...
                ,'1 1:0.24 2:0.15\n'...
                ,'1 1:0.67 2:0.24\n'...
                ,'1 1:0.12 2:0.89\n'...
                ,'1 1:0.52       \n'...
                ,'1 1:0.21 2:0.97\n']);
    
    k = 3;
    % If the dataset has M samples, then IDX is an Mx1 matrix assigning each sample the closest cluster index.   
    [ IDX ] = fcl_kmeans(X, k);
    
    % Second example:
    % Additionally to IDX, the cluster centers matrix C is outputted.
    [ IDX, C ] = fcl_kmeans(X, k);
    
    % Third example:
    % Additionally to IDX and C, sumd the within-cluster sums of point-to-centroid distances in the 1-by-k vector is returned 
    [ IDX, C, sumd] = fcl_kmeans(X, k);
end
