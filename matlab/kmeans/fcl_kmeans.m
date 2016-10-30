%  fcl kmeans algorithm.
% 
%  [IDX]  = fcl_kmeans(X, k, opts) returns for every point the closest cluster center index.
% 
%  [IDX, C] = fcl_kmeans(X, k, opts) returns the cluster center matrix C.
%
%  [IDX, C, SUMD] = fcl_kmeans(X, k, opts) returns the sum of cluster distances for every cluster center as a 1 by k array.
%
%  [IDX, C, SUMD, T] = fcl_kmeans(X, k, opts) returns internal information about the algorithm (explanation see below).
%
%  Input:
%       X:      sparse input data (num columns, dim rows)
%       k:      the desired number of clusters
%       opts:   (Optional) struct containing the following additional parameters:
%
%               opts.algorithm: Choices are "kmeans", "kmeans_optimized" (default), "yinyang", 
%                               "fast_yinyang", "minibatch_kmeans", "minibatch_kmeans_optimized", "elkan"
%               opts.init:      Initialization strategy. Choices are "random" (default) and "kmeans++".              
%               opts.seed:      Seed when using random initializations. Default is 1.
%               opts.tol:       Tolerance (stopping criterion). Default is 1e-6.
%               opts.max_iter:  The iteration limit. Default is 1000.    
%               opts.silent:    Suppress console output. Default is false.
%               opts.no_cores:  Specify how many cores to use. Default = all.
%               opts.remove_empty:  Remove empty clusters at the end of the clustering (default: false).
%
%
%  Output T aka tracking parameters:
%       T.general_params                   % stores all params kmeans was run with
%       T.general_params.no_clusters       % number of clusters requested
%       T.general_params.algorithm         % the used algorithms
%       T.general_params.seed              % The seed used for clustering
%       T.general_params.remove_empty      % If true, empty clusters are removed after clustering
%       T.general_params.iteration_limit   % After how many iterations to stop (if not converged by then)
%       T.general_params.tol               % If objective does not improve more than 'tol', converge
%       T.general_params.init              % Initialization strategy used
%       T.general_params.no_cores_used     % Number of cores used for this experiment
% 
%       T.duration_init                    % Duration the initialization strategy took
%       T.initial_wcssd                    % Objective value before first iteration
%       T.input_samples                    % Number of samples in the dataset used for clustering
%       T.input_dimension                  % Dimensionality of the input dataset
%       T.additional_params                % Additional internal params set by the algorithms (can be modified from outside)
%       T.block_vector_data                % Data about block vector 
%       T.iteration_bv_calcs               % This field only exists if a block vector algorithm is used and contains an for every iteration the amount of bv calculations
%       T.iteration_wcssd                  % Contains for every iteration the objective value
%       T.iteration_changes                % Contains how many samples moved to a new cluster in every iteration
%       T.iteration_remaining_clusters     % Number of non empty clusters in every iteration.
%       T.iteration_full_distance_calcs    % Number of full distance calculations per iteration.
%       T.iteration_durations_calcs        % Time it took to do the main loop of calculation
%       T.iteration_durations_update_clusters  % Time it took to move the clusters
%       T.iteration_durations              % Time the complete iteration took
%       T.no_iterations                    % Number of iterations needed to converge
%       T.duration_kmeans                  % Time the complete kmeans clustering took
%       T.no_clusters_remaining            % Number of clusters remaining (s. remove_empty option)
%
%  Tracking parameters are a very powerful tool for researchers and can even be extended with very little effort to output
%  arbitrary data about an algorithm. Have a look at fcl/examples/matlab/kmeans/basic/tracking_params.m for more information.
%
%
%  For more information about the implemented algorithms, see
%
%  Thomas Bottesch, Thomas Buehler, Markus Kaechele
%  Speeding up k-means by approximating Euclidean distances via blockvectors
%  Proc. 33rd International Conference on Machine Learning, New York, USA, 2016. 
%  JMLR: W&CP volume 48. http://jmlr.org/proceedings/papers/v48/bottesch16.pdf
