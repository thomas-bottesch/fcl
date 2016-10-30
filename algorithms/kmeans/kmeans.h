#ifndef KMEANS_H
#define KMEANS_H

#include "kmeans_control.h"

/**
 * @brief The kmeans and kmeans_optimized algorithm.
 *
 * @param samples which shall be clustered
 * @param prms are the parameters to control the clustering
 * @return
 */
struct csr_matrix* kmeans_optimized(struct csr_matrix* samples, struct kmeans_params *prms);

#endif
