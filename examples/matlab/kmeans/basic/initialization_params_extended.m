function initialization_params_extended()
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

    opts.silent = true;
    opts.seed = 4;
    opts.max_iter = 1;
    opts.init = 'initialization_params';
    
    % This example goes iteration by iteration through k-means by
    % resuming at the last iteration. If there is no more change we stop!
    
    k = 3;
    i = 0;
    %[ IDX, C ] = fcl_kmeans(X, k, opts);
    while 1
        [IDX, C, SUMD, T, INITPRMS] = fcl_kmeans(X, k, opts);
        opts.initprms = INITPRMS;
        g=sprintf('%d ', transpose(opts.initprms.assignments));
        i = i + 1;
        fprintf('Assignments (iter: %d): %s\n', i, g)
        
        if T.iteration_changes(1) == 0
            fprintf('Kmeans finished cause there was no change in the last two assignments!\n')
            break;
        end
    end
end
