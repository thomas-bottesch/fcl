#include "kmeans_control.h"
#include "kmeans.h"
#include "yinyang.h"
#include "minibatch_kmeans.h"
#include "pca_minibatch_kmeans.h"
#include "elkan_kmeans.h"
#include "pca_kmeans.h"
#include "pca_elkan_kmeans.h"
#include "pca_yinyang.h"
#include "kmeanspp.h"

const char *KMEANS_ALGORITHM_NAMES[NO_KMEANS_ALGOS] = {"kmeans"
										  , "bv_kmeans"
                                          , "bv_kmeans_ondemand"
										  , "yinyang"
										  , "bv_yinyang"
                                          , "bv_yinyang_ondemand"
										  , "minibatch_kmeans"
										  , "bv_minibatch_kmeans"
										  , "pca_minibatch_kmeans"
										  , "elkan"
										  , "bv_elkan"
										  , "bv_elkan_ondemand"
	                                      , "pca_elkan"
	                                      , "pca_yinyang"
										  , "pca_kmeans"
										  , "kmeans++"
										  , "bv_kmeans++"
										  , "pca_kmeans++"};

const char *KMEANS_ALGORITHM_DESCRIPTION[NO_KMEANS_ALGOS] = {"standard k-means"
											  , "k-means optimized (with block vectors)"
                                              , "k-means optimized (with block vectors calculated on demand, a bit slower but needs a lot less RAM)"
											  , "yinyang k-means"
											  , "yinyang k-means (with block vectors)"
                                              , "yinyang k-means (with block vectors calculated on demand, a bit slower but needs a lot less RAM)"
											  , "minibatch k-means"
											  , "minibatch k-means (with block vectors)"
											  , "minibatch k-means (with pca lower bounds)"
											  , "triangle inequality optimized kmeans"
											  , "triangle inequality optimized kmeans (with block vectors)"
											  , "triangle inequality optimized kmeans (with block vectors calculated on demand, a bit slower but needs a lot less RAM)"
											  , "triangle inequality optimized kmeans with pca lower bounds"
											  , "yinyang k-means with pca lower bounds"
											  , "k-means with pca lower bounds"
											  , "kmeans++ as full clustering strategy (not just init)"
											  , "kmeans++ (with block vectors)"
											  , "kmeans++ (with pca lower bounds)"};

kmeans_algorithm_function KMEANS_ALGORITHM_FUNCTIONS[NO_KMEANS_ALGOS] = {bv_kmeans
														  , bv_kmeans
														  , bv_kmeans
														  , yinyang_kmeans
														  , yinyang_kmeans
														  , yinyang_kmeans
														  , bv_minibatch_kmeans
														  , bv_minibatch_kmeans
														  , pca_minibatch_kmeans
                                                          , elkan_kmeans
                                                          , elkan_kmeans
                                                          , elkan_kmeans
                                                          , pca_elkan_kmeans
                                                          , pca_yinyang_kmeans
                                                          , pca_kmeans
                                                          , bv_kmeanspp
                                                          , bv_kmeanspp
                                                          , bv_kmeanspp};

const char *KMEANS_INIT_NAMES[NO_KMEANS_INITS] = {"random"
                                                  , "kmeans++"
                                                  , "initialization_params"};

const char *KMEANS_INIT_DESCRIPTION[NO_KMEANS_INITS] = {"initial cluster centers are random samples from input matrix",
                                  	  	                "initial cluster centers are samples from input matrix chosen by kmeans++ strategy",
                                                        "a list with len(list) = len(samples) is supplied which assigns each sample to a (subset of samples) = cluster centers"};

