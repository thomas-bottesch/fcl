#ifndef PCA_KMEANS_ELKAN_H
#define PCA_KMEANS_ELKAN_H

#include "kmeans_control.h"

/**
 * @brief The pca elkan version of k-means.
 *
 * @param samples which shall be clustered
 * @param prms are the parameters to control the clustering
 * @return
 */
struct csr_matrix* pca_elkan_kmeans(struct csr_matrix* samples, struct kmeans_params *prms);

#endif
