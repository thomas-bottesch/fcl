#include "kmeans.h"
#include "kmeans_utils.h"

#ifndef ELKAN_COMMONS_H
#define ELKAN_COMMONS_H

void calculate_cluster_distance_matrix(struct general_kmeans_context* ctx
                                       , VALUE_TYPE** dist_clusters_clusters
                                       , VALUE_TYPE* min_dist_cluster_clusters
                                       , uint32_t* stop);
                                       
#endif