function tracking_params()
    addpath(fileparts(fileparts(fileparts(mfilename('fullpath')))));
    check_compile('kmeans')
    
    % This example shows how to retrieve tracking params from a kmeans run.
    % The tracking params can be used by developers to evaluate how many
    % Calculations per iteration are happening 
    X = sparse(rand(100,100));
    
    k = 10;
    % Third example:
    % Additionally to IDX, C, sumd: Get the tracked parameters.
    [ IDX, C, sumd, tracked_params] = fcl_kmeans(X, k);
    
    % tracked params is a struct with various items

    % tracked_params.general_params                   % stores all params kmeans was run with
    % tracked_params.general_params.no_clusters       % number of clusters requested
    % tracked_params.general_params.algorithm         % the used algorithms
    % tracked_params.general_params.seed              % The seed used for clustering
    % tracked_params.general_params.remove_empty      % If true, empty clusters are removed after clustering
    % tracked_params.general_params.iteration_limit   % After how many iterations to stop (if not converged by then)
    % tracked_params.general_params.tol               % If objective does not improve more than 'tol', converge
    % tracked_params.general_params.init              % Initialization strategy used
    % tracked_params.general_params.no_cores_used     % Number of cores used for this experiment
    % 
    % tracked_params.duration_init                    % Duration the initialization strategy took
    % tracked_params.initial_wcssd                    % Objective value before first iteration
    % tracked_params.input_samples                    % Number of samples in the dataset used for clustering
    % tracked_params.input_dimension                  % Dimensionality of the input dataset
    % tracked_params.additional_params                % Additional internal params set by the algorithms (can be modified from outside)
    % tracked_params.block_vector_data                % Data about block vector 
    % tracked_params.iteration_bv_calcs               % This field only exists if a block vector algorithm is used and contains an for every iteration the amount of bv calculations
    % tracked_params.iteration_wcssd                  % Contains for every iteration the objective value
    % tracked_params.iteration_changes                % Contains how many samples moved to a new cluster in every iteration
    % tracked_params.iteration_remaining_clusters     % Number of non empty clusters in every iteration.
    % tracked_params.iteration_full_distance_calcs    % Number of full distance calculations per iteration.
    % tracked_params.iteration_durations_calcs        % Time it took to do the main loop of calculation
    % tracked_params.iteration_durations_update_clusters  % Time it took to move the clusters
    % tracked_params.iteration_durations              % Time the complete iteration took
    % tracked_params.no_iterations                    % Number of iterations needed to converge
    % tracked_params.duration_kmeans                  % Time the complete kmeans clustering took
    % tracked_params.no_clusters_remaining            % Number of clusters remaining (s. remove_empty option)
    
    
    % tracked_params is a completely dynamic structure, a perfect tool to
    % parametrize a new algorithm or retrieve information from an
    % algorithm.
    % 
    % As a developer they give you the possibility to do the following in
    % the source code of e.g. kmeans.c
    %
    % d_add_ilist(&(prms->tr), "new_list", 1);
    % d_add_ilist(&(prms->tr), "new_list", 2);
    % d_add_ilist(&(prms->tr), "new_list", 3);
    %
    % Then recompiling creates automatically the new field:
    % tracked_params.new_list with the matrix [1; 2; 3]
    % 
    % No other file has to be modified 
    
    
    % additional information can be added from the outside which are then
    % available inside the algorithms. e.g.
    
    opts.info.dataset_id = 'test';
    [ IDX, C, sumd, tracked_params] = fcl_kmeans(X, k, opts);
    
    % dataset_id can be used by the algorithms and in the end it will be
    % written to the tracked_params as well.
    tracked_params.info
    
    % While values added as info to the options can only contain strings
    % Another special field in options lets us pass numeric parameters.
    
    opts.additional_params.new_param = 0.125;
    [ IDX, C, sumd, tracked_params] = fcl_kmeans(X, k, opts);
    
    % developers can access this setting immediately in the source code by
    % doing e.g. in kmeans.c the following:
    % VALUE_TYPE new_param = d_get_subfloat_default(&(prms->tr, "additional_params", "new_param", 0.3);
    
    % If the param 'new_param' was set in the options struct, then it will
    % be used in the c code. Else the default of 0.3 is used as new_param.
    % This is the way all additional params should be set.
    
    % After clustering the tracked params also contain the 'new_param'.
    % Even if you did not specify the 'new_param' in the options. And the
    % C code accesses 'd_get_subfloat_default' the default parameter will
    % show up in the tracked_params.
    % There is no need to ever hardcode and recompile the C code to try
    % different values for parameters.
    tracked_params.additional_params
end
