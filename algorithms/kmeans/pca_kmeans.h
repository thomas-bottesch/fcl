#ifndef PCA_KMEANS_H
#define PCA_KMEANS_H

#include "kmeans_control.h"

/**
 * @brief The k-means algorithm which uses pca lowerbounds.
 *
 * @param samples which shall be clustered
 * @param prms are the parameters to control the clustering
 * @return
 */
struct kmeans_result* pca_kmeans(struct csr_matrix* samples, struct kmeans_params *prms);

#endif
