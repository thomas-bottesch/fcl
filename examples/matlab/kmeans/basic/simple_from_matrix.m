function simple_from_matrix()
    addpath(fileparts(fileparts(fileparts(mfilename('fullpath')))));
    check_compile('kmeans')
    
    % This example shows how to cluster a matlab matrix
    % Only sparse matrices with double values are supported!
    X = sprand(1000, 200, 1/10);
    
    k = 10;
    % If the dataset has M samples, then IDX is an Mx1 matrix assigning each sample the closest cluster index.   
    [ IDX ] = fcl_kmeans(X, k);
    
    % Second example:
    % Additionally to IDX, the cluster centers matrix C is outputted.
    [ IDX, C ] = fcl_kmeans(X, k);
    
    % Third example:
    % Additionally to IDX and C, sumd the within-cluster sums of point-to-centroid distances in the 1-by-k vector is returned 
    [ IDX, C, sumd] = fcl_kmeans(X, k);
end
