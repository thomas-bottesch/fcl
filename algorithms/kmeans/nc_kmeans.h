#ifndef NC_KMEANS_H
#define NC_KMEANS_H

#include "kmeans_control.h"

/**
 * @brief The kmeans and nc_kmeans algorithm.
 *
 * @param samples which shall be clustered
 * @param prms are the parameters to control the clustering
 * @return
 */
struct kmeans_result* nc_kmeans(struct csr_matrix* samples, struct kmeans_params *prms);

#endif
