% Fast k-means clustering library.
% 
%  Usage: C = fcl_kmeans(X, k, opts) 
% 
%  Input:
%       X:      sparse input data (num columns, dim rows)
%       k:      the desired number of clusters
%       opts:   (Optional) struct containing the following additional parameters:
%
%               opts.algorithm: Choices are "kmeans", "kmeans_optimized" (default), "yinyang", 
%                               "fast_yinyang", "minibatch_kmeans", "minibatch_kmeans_optimized"
%               opts.init:      Initialization strategy. Choices are "random" (default) and "kmeans++".              
%               opts.seed:      Seed when using random initializations. Default is 1.
%               opts.tol:       Tolerance (stopping criterion). Default is 1e-6.
%               opts.max_iter:  The iteration limit. Default is 1000.    
%               opts.silent:    Suppress console output. Default is false.
%               opts.no_cores:  Specify how many cores to use. Default = all.
%
% For more information, see
%
%  Thomas Bottesch, Thomas Buehler, Markus Kaechele
%  Speeding up k-means by approximating Euclidean distances via blockvectors
%  Proc. 33rd International Conference on Machine Learning, New York, USA, 2016. 
%  JMLR: W&CP volume 48. http://jmlr.org/proceedings/papers/v48/bottesch16.pdf

