#ifndef KMEANSPP_H
#define KMEANSPP_H

#include "kmeans_control.h"

/**
 * @brief The kmeans and bv_kmeans algorithm.
 *
 * @param samples which shall be clustered
 * @param prms are the parameters to control the clustering
 * @return
 */
struct kmeans_result* bv_kmeanspp(struct csr_matrix* samples, struct kmeans_params *prms);

#endif
