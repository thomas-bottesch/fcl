#include "kmeans.h"
#include "kmeans_utils.h"
#include "../../utils/matrix/csr_matrix/csr_to_vector_list.h"
#include "../../utils/matrix/vector_list/vector_list_math.h"
#include "../../utils/matrix/csr_matrix/csr_math.h"
#include "../../utils/vector/common/common_vector_math.h"
#include "../../utils/vector/sparse/sparse_vector_math.h"
#include "../../utils/fcl_logging.h"

#include <math.h>
#include <unistd.h>
#include <float.h>

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
                if (!(ctx->clusters_not_changed[i] && ctx->clusters_not_changed[i])) {
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
struct csr_matrix* elkan_kmeans(struct csr_matrix* samples, struct kmeans_params *prms) {

    uint64_t i;
    uint64_t j;
    uint64_t k;
    struct csr_matrix* resulting_clusters;
    struct general_kmeans_context ctx;


    char* bound_needs_update;                           /* bool per sample, 1 if a bound needs updating (rx)*/
    VALUE_TYPE** lb_samples_clusters;                   /* lower bounds for every sample to every cluster (lxc) */
    VALUE_TYPE** dist_clusters_clusters;                /* distance from, to every cluster (dcc) */
    VALUE_TYPE*  min_dist_cluster_clusters;             /* minimum distance between a cluster and all other clusters (sc) */
    VALUE_TYPE*  distance_clustersold_to_clustersnew;   /* distance between clusters before/after a shift */
    /* upper bounds from samples to clusters are stored in ctx.cluster_distances */

    initialize_general_context(prms, &ctx, samples);

    /* initialization of the triangle inequality boundaries */
    bound_needs_update = (char*) calloc(ctx.samples->sample_count, sizeof(char));

    lb_samples_clusters = (VALUE_TYPE**) calloc(ctx.samples->sample_count, sizeof(VALUE_TYPE*));
    for (i = 0; i < ctx.samples->sample_count; i++) {
        lb_samples_clusters[i] = (VALUE_TYPE*) calloc(ctx.no_clusters, sizeof(VALUE_TYPE));
        /* initialize the lower bounds to the distance of sample i to the
         * initial chosen cluster ctx.cluster_assignments[i]
         */
        lb_samples_clusters[i][ctx.cluster_assignments[i]] = ctx.cluster_distances[i];
        bound_needs_update[i] = 0;
    }

    dist_clusters_clusters = (VALUE_TYPE**) calloc(ctx.no_clusters, sizeof(VALUE_TYPE*));
    for (i = 0; i < ctx.no_clusters; i++) {
        dist_clusters_clusters[i] = (VALUE_TYPE*) calloc(ctx.no_clusters, sizeof(VALUE_TYPE));
    }

    min_dist_cluster_clusters = (VALUE_TYPE*) calloc(ctx.no_clusters, sizeof(VALUE_TYPE));
    distance_clustersold_to_clustersnew = (VALUE_TYPE*) calloc(ctx.no_clusters, sizeof(VALUE_TYPE));

    for (i = 0; i < prms->iteration_limit && !ctx.converged && !prms->stop; i++) {

        /* initialize data needed for the iteration */
        pre_process_iteration(&ctx);

        calculate_cluster_distance_matrix(&ctx, dist_clusters_clusters, min_dist_cluster_clusters, &(prms->stop));

        #pragma omp parallel for schedule(dynamic, 1000)
        for (j = 0; j < ctx.samples->sample_count; j++) {
            /* iterate over all samples */
            VALUE_TYPE dist;
            uint64_t cluster_id, sample_id;

            sample_id = j;

            if (omp_get_thread_num() == 0) check_signals(&(prms->stop));

            /* we identified that for this sample no closer cluster can be found */
            if (ctx.cluster_distances[sample_id]
                    <= min_dist_cluster_clusters[ctx.cluster_assignments[sample_id]]) {
                /* there cannot be any cluster closer than the current one */
                continue;
            }

            if (!prms->stop) {
                for (cluster_id = 0; cluster_id < ctx.no_clusters; cluster_id++) {
                    /* iterate over all cluster centers */

                    /* if we are not in the first iteration and this cluster is empty, continue to next cluster */
                    if (i != 0 && ctx.cluster_counts[cluster_id] == 0) continue;
                    if (cluster_id == ctx.previous_cluster_assignments[sample_id]) continue;
                    if (ctx.cluster_distances[sample_id] <= lb_samples_clusters[sample_id][cluster_id]) continue;
                    if (ctx.cluster_distances[sample_id] <= 0.5 * dist_clusters_clusters[ctx.cluster_assignments[sample_id]][cluster_id]) continue;

                    if (bound_needs_update[sample_id]) {
                        /* if we reached this point we need to calculate a full euclidean distance */
                        dist = euclid_vector_list(ctx.samples, sample_id, ctx.cluster_vectors, ctx.cluster_assignments[sample_id]
                                , ctx.vector_lengths_samples, ctx.vector_lengths_clusters);
                        ctx.done_calculations += 1;

                        /* update lower bound */
                        lb_samples_clusters[sample_id][ctx.cluster_assignments[sample_id]] = dist;

                        /* tighten upper bound */
                        ctx.cluster_distances[sample_id] = dist;

                        /* remember that the bounds were updated */
                        bound_needs_update[sample_id] = 0;
                    }

                    if (ctx.cluster_distances[sample_id] > lb_samples_clusters[sample_id][cluster_id]
                        || ctx.cluster_distances[sample_id] > 0.5 * dist_clusters_clusters[ctx.cluster_assignments[sample_id]][cluster_id]) {

                        dist = euclid_vector_list(ctx.samples, sample_id, ctx.cluster_vectors, cluster_id
                                                    , ctx.vector_lengths_samples, ctx.vector_lengths_clusters);
                        ctx.done_calculations += 1;

                        /* tighten lower bound */
                        lb_samples_clusters[sample_id][cluster_id] = dist;

                        if (dist < ctx.cluster_distances[sample_id]) {
                            /* replace current best distance with new distance */
                            ctx.cluster_distances[sample_id] = dist;
                            ctx.cluster_assignments[sample_id] = cluster_id;
                        }
                    }
                }
            }
        }

        post_process_iteration(&ctx, prms);

        /* shift clusters to new position */
        calculate_shifted_clusters(&ctx);

        /* calculate distance between a cluster before and after the shift */
        calculate_distance_clustersold_to_clustersnew(distance_clustersold_to_clustersnew
                                                      , ctx.shifted_cluster_vectors
                                                      , ctx.cluster_vectors
                                                      , ctx.no_clusters
                                                      , ctx.vector_lengths_shifted_clusters
                                                      , ctx.vector_lengths_clusters
                                                      , ctx.clusters_not_changed);

        switch_to_shifted_clusters(&ctx);

        #pragma omp parallel for private(j)
        for(k = 0; k < ctx.samples->sample_count; k++) {
            for(j = 0; j < ctx.no_clusters; j++) {
                if (!ctx.clusters_not_changed[j]) {
                    lb_samples_clusters[k][j] = ((lb_samples_clusters[k][j] - distance_clustersold_to_clustersnew[j] > 0) ?
                                                (lb_samples_clusters[k][j] - distance_clustersold_to_clustersnew[j]) :
                                                0);
                }
            }

            if (!ctx.clusters_not_changed[ctx.cluster_assignments[k]]) {
                ctx.cluster_distances[k] = ctx.cluster_distances[k] + distance_clustersold_to_clustersnew[ctx.cluster_assignments[k]];
                bound_needs_update[k] = 1;
            }
        }

        print_iteration_summary(&ctx, prms, i);
    }

    if (prms->verbose) LOG_INFO("total total_no_calcs = %" PRINTF_INT64_MODIFIER "u", ctx.total_no_calcs);

    resulting_clusters = create_result_clusters(prms, &ctx);

    /* cleanup all */
    free_general_context(&ctx, prms);
    free_null(bound_needs_update);

    for (i = 0; i < ctx.samples->sample_count; i++) {
        free_null(lb_samples_clusters[i]);
    }
    free_null(lb_samples_clusters);

    for (i = 0; i < ctx.no_clusters; i++) {
        free_null(dist_clusters_clusters[i]);
    }
    free_null(dist_clusters_clusters);
    free_null(min_dist_cluster_clusters);
    free_null(distance_clustersold_to_clustersnew);

    return resulting_clusters;
}
