#ifndef PCA_YINYANG_H
#define PCA_YINYANG_H

#include "kmeans_control.h"

/**
 * @brief The pca yinyang kmeans algorithm.
 *
 * @param samples which shall be clustered
 * @param prms are the parameters to control the clustering
 * @return
 */
struct csr_matrix* pca_yinyang_kmeans(struct csr_matrix* samples, struct kmeans_params *prms);

#endif
