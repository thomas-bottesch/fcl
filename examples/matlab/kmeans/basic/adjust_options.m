function adjust_options()
    addpath(fileparts(fileparts(fileparts(mfilename('fullpath')))));
    check_compile('kmeans')
    
    % This example shows how to cluster a matlab matrix
    % Only sparse matrices with double values are supported!
    X = sprand(1000, 500, 1/10);

    %    KEY_TYPE kmeans_algorithm_id;   /**< id specifies which k-means algorithm to use */   
    opts.seed = 5;                  % change starting position of clustering
    opts.algorithm = 'yinyang';     % change the algorithm to yinyang
    opts.init = 'random';           % use random samples as initialization
    opts.no_cores = 1;              % number of cores to use. for scientific experiments always use 1!
    opts.max_iter = 10;             % stop after 10 iterations
    opts.tol = 1e-5;                % change the tolerance to converge quicker
    opts.silent = true;             % do not output anything while clustering
    opts.remove_empty = true;       % remove empty clusters from resulting cluster center matrix
    
    % specifying options is optional. there is no need to specify all
    % options!
    
    k = 99;
    [ IDX, C ] = fcl_kmeans(X, k, opts);
    [ dim_C, no_samples_C] = size(C);
    fprintf('remaining clusters after removing empty ones: %i, cluster dimensions %i\n', no_samples_C, dim_C);

    
    
    % second example
    new_opts.silent = true;
    new_opts.algorithm = 'kmeans_optimized';     % change the algorithm to kmeans_optimized
    new_opts.additional_params.bv_annz = 0.125;  % modify internal parameters of the algorithm
    [ IDX, C ] = fcl_kmeans(X, k, new_opts);
end
