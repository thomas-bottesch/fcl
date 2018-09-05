#include "kmeans_control.h"
#include "kmeans.h"
#include "yinyang.h"
#include "minibatch_kmeans.h"
#include "elkan_kmeans.h"
#include "pca_kmeans.h"
#include "pca_elkan_kmeans.h"
#include "pca_yinyang.h"

const char *KMEANS_ALGORITHM_NAMES[NO_KMEANS_ALGOS] = {"kmeans"
										  , "kmeans_optimized"
                                          , "kmeans_optimized_ondemand"
										  , "yinyang"
										  , "fast_yinyang"
                                          , "fast_yinyang_ondemand"
										  , "minibatch_kmeans"
										  , "minibatch_kmeans_optimized"
										  , "elkan"
										  , "elkan_optimized"
										  , "elkan_optimized_ondemand"
	                                      , "pca_elkan"
	                                      , "pca_yinyang"
										  , "pca_kmeans"};

const char *KMEANS_ALGORITHM_DESCRIPTION[NO_KMEANS_ALGOS] = {"standard k-means"
											  , "k-means optimized (with block vectors)"
                                              , "k-means optimized (with block vectors calculated on demand, a bit slower but needs a lot less RAM)"
											  , "yinyang k-means"
											  , "yinyang k-means (with block vectors)"
                                              , "yinyang k-means (with block vectors calculated on demand, a bit slower but needs a lot less RAM)"
											  , "minibatch k-means"
											  , "minibatch k-means (with block vectors)"
											  , "triangle inequality optimized kmeans"
											  , "triangle inequality optimized kmeans (with block vectors)"
											  , "triangle inequality optimized kmeans (with block vectors calculated on demand, a bit slower but needs a lot less RAM)"
											  , "triangle inequality optimized kmeans with pca lower bounds"
											  , "yinyang k-means with pca lower bounds"
											  , "k-means with pca lower bounds"};

kmeans_algorithm_function KMEANS_ALGORITHM_FUNCTIONS[NO_KMEANS_ALGOS] = {kmeans_optimized
														  , kmeans_optimized
														  , kmeans_optimized
														  , yinyang_kmeans
														  , yinyang_kmeans
														  , yinyang_kmeans
														  , minibatch_kmeans_optimized
														  , minibatch_kmeans_optimized
                                                          , elkan_kmeans
                                                          , elkan_kmeans
                                                          , elkan_kmeans
                                                          , pca_elkan_kmeans
                                                          , pca_yinyang_kmeans
                                                          , pca_kmeans};

const char *KMEANS_INIT_NAMES[NO_KMEANS_INITS] = {"random"
                                  , "kmeans++"};

const char *KMEANS_INIT_DESCRIPTION[NO_KMEANS_INITS] = {"initial cluster centers are random samples from input matrix"
                                  	  	  , "initial cluster centers are samples from input matrix chosen by kmeans++ strategy"};

