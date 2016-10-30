function fit_assign_separate()
    addpath(fileparts(fileparts(fileparts(mfilename('fullpath')))));
    check_compile('kmeans')
    
    % This example shows how to cluster a matlab matrix with fcl_kmeans_fit
    % At a later point fcl_kmeans_predict is called to assign other samples
    % to the found centers C.
    X = sprand(1000, 1000, 1/10);
    
    k = 10;
    % The cluster centers matrix C is outputted.
    [ C ] = fcl_kmeans_fit(X, k);
    
    % Create new test samples
    TEST = sparse(rand(100,100));
    
    % Assign test samples to the cluster centers.
    [ IDX ] = fcl_kmeans_predict(C, TEST);
end
