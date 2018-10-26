#ifndef MINIBATCH_KMEANS_H
#define MINIBATCH_KMEANS_H

#include "kmeans_control.h"

/**
 * @brief The minibatch kmeans and minibatch bv_kmeans algorithm.
 *
 * @param samples which shall be clustered
 * @param prms are the parameters to control the clustering
 * @return
 */
struct csr_matrix* bv_minibatch_kmeans(struct csr_matrix* samples
                                             , struct kmeans_params *prms);

#endif
