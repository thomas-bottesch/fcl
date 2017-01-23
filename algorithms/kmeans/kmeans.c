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
struct csr_matrix* kmeans_optimized(struct csr_matrix* samples, struct kmeans_params *prms) {

    uint32_t i;
    uint64_t j;
    uint64_t keys_per_block;
    uint64_t block_vectors_dim;         /* size of block vectors */
    VALUE_TYPE desired_bv_annz;         /* desired size of the block vectors */
    struct csr_matrix block_vectors_samples;  /* block vector matrix of samples */
    struct sparse_vector* block_vectors_clusters; /* block vector matrix of clusters */
    struct csr_matrix* resulting_clusters;
    uint32_t disable_optimizations;

    /* kmeans_optimized: contains all samples which are eligible for the cluster
     * no change optimization.
     */
    uint32_t *eligible_for_cluster_no_change_optimization;
    struct general_kmeans_context ctx;

    initialize_general_context(prms, &ctx, samples);

    desired_bv_annz = d_get_subfloat_default(&(prms->tr)
                                            , "additional_params", "bv_annz", 0.3);
    block_vectors_dim = 0;
    keys_per_block = 0;
    disable_optimizations = prms->kmeans_algorithm_id == ALGORITHM_KMEANS;

    if (!disable_optimizations) {
        initialize_csr_matrix_zero(&block_vectors_samples);

        if (prms->kmeans_algorithm_id == ALGORITHM_KMEANS_OPTIMIZED) {
            /* search for a suitable size of the block vectors for the input samples and create them */
            search_samples_block_vectors(prms, ctx.samples, desired_bv_annz
                                         , &block_vectors_samples
                                         , &block_vectors_dim);
        }

        if (prms->kmeans_algorithm_id == ALGORITHM_KMEANS_OPTIMIZED_ONDEMAND) {
            block_vectors_dim = search_block_vector_size(ctx.samples, desired_bv_annz, prms->verbose);

            keys_per_block = ctx.samples->dim / block_vectors_dim;
            if (ctx.samples->dim % block_vectors_dim > 0) keys_per_block++;
        }

        /* create block vectors for the clusters */
        create_block_vectors_list_from_vector_list(ctx.cluster_vectors
                                                        , block_vectors_dim
                                                        , ctx.no_clusters
                                                        , ctx.samples->dim
                                                        , &block_vectors_clusters);
    }

    eligible_for_cluster_no_change_optimization = (uint32_t*) calloc(ctx.samples->sample_count, sizeof(uint32_t));

    for (i = 0; i < prms->iteration_limit && !ctx.converged && !prms->stop; i++) {
        /* track how many blockvector calculations were made / saved */
        uint64_t saved_calculations_bv, saved_calculations_prev_cluster;
        uint64_t done_blockvector_calcs, saved_calculations_cauchy;

        /* reset all calculation counters */
        done_blockvector_calcs = 0;
        saved_calculations_cauchy = 0;
        saved_calculations_prev_cluster = 0;
        saved_calculations_bv = 0;

        /* initialize data needed for the iteration */
        pre_process_iteration(&ctx);

        #pragma omp parallel for schedule(dynamic, 1000)
        for (j = 0; j < ctx.samples->sample_count; j++) {
            /* iterate over all samples */

            VALUE_TYPE dist;
            uint64_t cluster_id, sample_id;
            struct sparse_vector bv;
            bv.nnz = 0;
            bv.keys = NULL;
            bv.values = NULL;

            if (omp_get_thread_num() == 0) check_signals(&(prms->stop));

            if (!prms->stop) {
                sample_id = j;

                for (cluster_id = 0; cluster_id < ctx.no_clusters; cluster_id++) {
                    /* iterate over all cluster centers */

                    /* if we are not in the first iteration and this cluster is empty, continue to next cluster */
                    if (i != 0 && ctx.cluster_counts[cluster_id] == 0) continue;

                    if (!disable_optimizations) {
                        /* kmeans_optimized */

                        /* we already know the distance to the cluster from last iteration */
                        if (cluster_id == ctx.previous_cluster_assignments[sample_id]) continue;

                        /* clusters which did not move in the last iteration can be skipped if the sample is eligible */
                        if (eligible_for_cluster_no_change_optimization[sample_id] && ctx.clusters_not_changed[cluster_id]) {
                            /* cluster did not move and sample was eligible for this check. distance to this cluster can not be less than to our best from last iteration */
                            saved_calculations_prev_cluster += 1;
                            goto end;
                        }

                        /* evaluate cauchy approximation. fast but not good */
                        dist = lower_bound_euclid(ctx.vector_lengths_clusters[cluster_id]
                                                  , ctx.vector_lengths_samples[sample_id]);

                        if (dist >= ctx.cluster_distances[sample_id]) {
                            /* approximated distance is larger than current best distance. skip full distance calculation */
                            saved_calculations_cauchy += 1;
                            goto end;
                        }
                        if (prms->kmeans_algorithm_id == ALGORITHM_KMEANS_OPTIMIZED) {
                            /* evaluate block vector approximation. */
                            dist = euclid_vector_list(&block_vectors_samples, sample_id
                                          , block_vectors_clusters, cluster_id
                                          , ctx.vector_lengths_samples
                                          , ctx.vector_lengths_clusters);
                        } else {
                            if (bv.keys == NULL) {
                                create_block_vector_from_csr_matrix_vector(ctx.samples
                                                                           , sample_id
                                                                           , keys_per_block
                                                                           , &bv);
                            }

                            dist = euclid_vector(bv.keys, bv.values, bv.nnz
                                                 , block_vectors_clusters[cluster_id].keys
                                                 , block_vectors_clusters[cluster_id].values
                                                 , block_vectors_clusters[cluster_id].nnz
                                                 , ctx.vector_lengths_samples[sample_id]
                                                 , ctx.vector_lengths_clusters[cluster_id]);
                        }

                        done_blockvector_calcs += 1;

                        if (dist >= ctx.cluster_distances[sample_id] && fabs(dist - ctx.cluster_distances[sample_id]) >= 1e-6) {
                            /* approximated distance is larger than current best distance. skip full distance calculation */
                            saved_calculations_bv += 1;
                            goto end;
                        }
                    }

                    /* if we reached this point we need to calculate a full euclidean distance */
                    dist = euclid_vector_list(ctx.samples, sample_id, ctx.cluster_vectors, cluster_id
                            , ctx.vector_lengths_samples, ctx.vector_lengths_clusters);

                    ctx.done_calculations += 1;

                    if (dist < ctx.cluster_distances[sample_id]) {
                        /* replace current best distance with new distance */
                        ctx.cluster_distances[sample_id] = dist;
                        ctx.cluster_assignments[sample_id] = cluster_id;
                    }
                    end:;
                }
            }

            if (!disable_optimizations) {
                free_null(bv.keys);
                free_null(bv.values);
            }
        }

        post_process_iteration(&ctx, prms);

        /* shift clusters to new position */
        calculate_shifted_clusters(&ctx);
        switch_to_shifted_clusters(&ctx);

        if (!disable_optimizations) {
            /* update only block vectors for cluster that shifted */
            update_changed_blockvectors(ctx.cluster_vectors
                                        , block_vectors_dim
                                        , ctx.no_clusters
                                        , ctx.samples->dim
                                        , ctx.clusters_not_changed
                                        , block_vectors_clusters);

            d_add_ilist(&(prms->tr), "iteration_bv_calcs", done_blockvector_calcs);
            d_add_ilist(&(prms->tr), "iteration_bv_calcs_success", saved_calculations_bv + saved_calculations_cauchy);

            #pragma omp parallel for
            for (j = 0; j < ctx.samples->sample_count; j++) {
                /* iterate over all samples */

                VALUE_TYPE previous_distance;
                previous_distance = ctx.cluster_distances[j];

                /* if the cluster did move. calculate the new distance to this sample */
                if (ctx.clusters_not_changed[ctx.cluster_assignments[j]] == 0) {
                    ctx.cluster_distances[j]
                      = euclid_vector_list(ctx.samples, j
                              , ctx.cluster_vectors, ctx.cluster_assignments[j]
                              , ctx.vector_lengths_samples
                              , ctx.vector_lengths_clusters);

                    /*#pragma omp critical*/
                    ctx.done_calculations += 1;
                    ctx.total_no_calcs += 1;
                }

                /* if the cluster moved towards this sample,
                 * then this sample is eligible to skip calculations to centers which
                 * did not move in the last iteration
                 */
                if (ctx.cluster_distances[j] <= previous_distance) {
                    eligible_for_cluster_no_change_optimization[j] = 1;
                } else {
                    eligible_for_cluster_no_change_optimization[j] = 0;
                }
            }
        } else {
            /* naive k-means without any optimization remembers nothing from
             * the previous iteration.
             */
            for (j = 0; j < ctx.samples->sample_count; j++) {
                ctx.cluster_distances[j] = DBL_MAX;
            }
        }

        print_iteration_summary(&ctx, prms, i);

        /* print block vector statistics */
        if (prms->verbose) LOG_INFO("BV statistics c:%" PRINTF_INT64_MODIFIER "u/b:%" PRINTF_INT64_MODIFIER "u/db:%" PRINTF_INT64_MODIFIER "u/pc:%" PRINTF_INT64_MODIFIER "u"
                , saved_calculations_cauchy
                , saved_calculations_bv
                , done_blockvector_calcs
                , saved_calculations_prev_cluster);
    }

    if (prms->verbose) LOG_INFO("total total_no_calcs = %" PRINTF_INT64_MODIFIER "u", ctx.total_no_calcs);

    resulting_clusters = create_result_clusters(prms, &ctx);

    /* cleanup all */
    if (!disable_optimizations) {
        free_csr_matrix(&block_vectors_samples);
        free_vector_list(block_vectors_clusters, ctx.no_clusters);
        free(block_vectors_clusters);
    }

    free_general_context(&ctx, prms);
    free_null(eligible_for_cluster_no_change_optimization);


    return resulting_clusters;
}
