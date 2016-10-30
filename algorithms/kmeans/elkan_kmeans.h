#ifndef KMEANS_ELKAN_H
#define KMEANS_ELKAN_H

#include "kmeans_control.h"

/**
 * @brief The elkan version of k-means.
 *
 * @param samples which shall be clustered
 * @param prms are the parameters to control the clustering
 * @return
 */
struct csr_matrix* elkan_kmeans(struct csr_matrix* samples, struct kmeans_params *prms);

#endif
