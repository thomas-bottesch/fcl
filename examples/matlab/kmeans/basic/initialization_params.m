function initialization_params()
    addpath(fileparts(fileparts(fileparts(mfilename('fullpath')))));
    check_compile('kmeans')
    
    % This example shows how to cluster a matlab matrix
    % Only sparse matrices with double values are supported!
    X = sprintf(['1 1:0.50 2:0.34\n'...
                ,'1 1:0.13 2:0.11\n'...
                ,'1 1:0.24 2:0.15\n'...
                ,'1 1:0.67 2:0.24\n'...
                ,'1 1:0.12 2:0.89\n'...
                ,'1 1:0.52       \n'...
                ,'1 1:0.21 2:0.97\n']);

    opts.initprms.assignments = uint64([0;0;1;0;2;1;3]);
    opts.initprms.initial_cluster_samples = uint64([2; 5; 1; 4]);
    opts.init = 'initialization_params';
    opts.silent = false;
    opts.seed = 1;
    
    % the initprms can be used to make kmeans start from a specific
    % position
    
    k = 2;
    %[ IDX, C ] = fcl_kmeans(X, k, opts);
    [IDX, C, SUMD, T, INITPRMS] = fcl_kmeans(X, k, opts);
    [ dim_C, no_samples_C] = size(C);
    fprintf('remaining clusters: %i\n', no_samples_C);
end
