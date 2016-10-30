#ifndef YINYANG_H
#define YINYANG_H

#include "kmeans_control.h"

/**
 * @brief The yinyang kmeans and fast yinyang kmeans algorithm.
 *
 * @param samples which shall be clustered
 * @param prms are the parameters to control the clustering
 * @return
 */
struct csr_matrix* yinyang_kmeans(struct csr_matrix* samples, struct kmeans_params *prms);

#endif
