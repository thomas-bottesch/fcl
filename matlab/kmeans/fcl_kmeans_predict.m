%  fcl kmeans predict
% 
%  [ IDX ]       = fcl_kmeans_predict(C, X, opts) returns for every point the closest cluster center index.
%
%  [ IDX, SUMD ] = fcl_kmeans_predict(C, X, opts) returns the sum of cluster distances for every cluster center as a 1 by k array.
% 
%  Input:
%       C:      matrix of cluster center as sparse input (num columns, dim rows)
%       X:      sparse input data (num columns, dim rows)
%       opts:   (Optional) struct containing the following additional parameters:
%
%               opts.silent:    Suppress console output. Default is false.
%               opts.no_cores:  Specify how many cores to use. Default = all.
%
% For more information, see
%
%  Thomas Bottesch, Thomas Buehler, Markus Kaechele
%  Speeding up k-means by approximating Euclidean distances via blockvectors
%  Proc. 33rd International Conference on Machine Learning, New York, USA, 2016. 
%  JMLR: W&CP volume 48. http://jmlr.org/proceedings/papers/v48/bottesch16.pdf

