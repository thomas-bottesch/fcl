#include "elkan_commons.h"
#include "../../utils/vector/sparse/sparse_vector_math.h"
#include "../../utils/global_defs.h"

void calculate_cluster_distance_matrix(struct general_kmeans_context* ctx
                                       , VALUE_TYPE** dist_clusters_clusters
                                       , VALUE_TYPE* min_dist_cluster_clusters
                                       , uint32_t* stop) {
    uint64_t i;
    uint64_t j;
    VALUE_TYPE dist_eval;

    #pragma omp parallel for
    for(i = 0; i < ctx->no_clusters; i++) {
        /* reset min_dist_cluster_clusters */
        min_dist_cluster_clusters[i] = VALUE_TYPE_MAX;
    }

    /* evaluate block vector approximation. */
    #pragma omp parallel for private(j)
    for(i = 0; i < ctx->no_clusters; i++) {
        if (*stop) continue;

        if (omp_get_thread_num() == 0) check_signals(stop);

        for(j = 0; j < ctx->no_clusters; j++) {
            if (i > j) {
                if (!(ctx->clusters_not_changed[i] && ctx->clusters_not_changed[j])) {
                    /* if none of the two clusters moved, dont recalculate the distance */
                    dist_clusters_clusters[i][j] = euclid_vector( ctx->cluster_vectors[i].keys
                                                                , ctx->cluster_vectors[i].values
                                                                , ctx->cluster_vectors[i].nnz
                                                                , ctx->cluster_vectors[j].keys
                                                                , ctx->cluster_vectors[j].values
                                                                , ctx->cluster_vectors[j].nnz
                                                                , ctx->vector_lengths_clusters[i]
                                                                , ctx->vector_lengths_clusters[j]);
                    ctx->done_calculations += 1;
                }

                dist_clusters_clusters[j][i] = dist_clusters_clusters[i][j];
                dist_eval = 0.5 * dist_clusters_clusters[i][j];
                min_dist_cluster_clusters[i] = (dist_eval < min_dist_cluster_clusters[i]) ? dist_eval : min_dist_cluster_clusters[i];
                min_dist_cluster_clusters[j] = (dist_eval < min_dist_cluster_clusters[j]) ? dist_eval : min_dist_cluster_clusters[j];
            }
        }
    }
}
