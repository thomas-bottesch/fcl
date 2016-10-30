function simple_from_file()
    addpath(fileparts(fileparts(fileparts(mfilename('fullpath')))));
    check_compile('kmeans')
    
    % This example shows how to cluster a dataset available at path_to_dataset in libsvm format
    % This is the most memory efficient way to cluster data, since the data only exists once in memory.
    % In contrast, when clustering from a matlab matrix the data resides twice in memory.
    path_to_dataset = get_dataset('example.scaled');
    fprintf('Dataset to cluster: %s \n', path_to_dataset);
    
    k = 10;
    % If the dataset has M samples, then IDX is an Mx1 matrix assigning each sample the closest cluster index.   
    [ IDX ] = fcl_kmeans(path_to_dataset, k);
    
    % Second example:
    % Additionally to IDX, the cluster centers matrix C is outputted.
    [ IDX, C ] = fcl_kmeans(path_to_dataset, k);
    
    % Third example:
    % Additionally to IDX and C, sumd the within-cluster sums of point-to-centroid distances in the 1-by-k vector is returned 
    [ IDX, C, sumd] = fcl_kmeans(path_to_dataset, k);
end
