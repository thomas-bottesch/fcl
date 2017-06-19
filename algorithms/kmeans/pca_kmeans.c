#include "pca_kmeans.h"
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

struct csr_matrix* pca_kmeans(struct csr_matrix* samples, struct kmeans_params *prms) {

    uint32_t i;
    uint64_t j;
    uint64_t keys_per_block;
    uint64_t block_vectors_dim;         /* size of block vectors */
    struct sparse_vector* pca_projection_samples;  /* block vector matrix of samples */
    struct sparse_vector* pca_projection_clusters; /* block vector matrix of clusters */
    struct csr_matrix* resulting_clusters;
    uint32_t disable_optimizations;

    VALUE_TYPE* vector_lengths_pca_samples;
    VALUE_TYPE* vector_lengths_pca_clusters;

    /* kmeans_optimized: contains all samples which are eligible for the cluster
     * no change optimization.
     */
    uint32_t *eligible_for_cluster_no_change_optimization;
    struct general_kmeans_context ctx;

    initialize_general_context(prms, &ctx, samples);

    block_vectors_dim = 0;
    keys_per_block = 0;
    disable_optimizations = prms->ext_vects == NULL;

    if (!disable_optimizations) {
        if (prms->kmeans_algorithm_id == ALGORITHM_PCA_KMEANS) {
            /* create pca projections for the samples */
            pca_projection_samples = matrix_dot(samples, prms->ext_vects);
            calculate_vector_list_lengths(pca_projection_samples, samples->sample_count, &vector_lengths_pca_samples);
        }

        /* create pca projections for the clusters */
        pca_projection_clusters = sparse_vectors_matrix_dot(ctx.cluster_vectors,
                                                            ctx.no_clusters,
                                                            prms->ext_vects);

        vector_lengths_pca_clusters = NULL;
    }

    eligible_for_cluster_no_change_optimization = (uint32_t*) calloc(ctx.samples->sample_count, sizeof(uint32_t));

    for (i = 0; i < prms->iteration_limit && !ctx.converged && !prms->stop; i++) {
        /* track how many blockvector calculations were made / saved */
        uint64_t saved_calculations_pca, saved_calculations_prev_cluster;
        uint64_t done_blockvector_calcs, saved_calculations_cauchy;

        /* reset all calculation counters */
        done_blockvector_calcs = 0;
        saved_calculations_cauchy = 0;
        saved_calculations_prev_cluster = 0;
        saved_calculations_pca = 0;

        /* initialize data needed for the iteration */
        pre_process_iteration(&ctx);

        free(vector_lengths_pca_clusters);
        calculate_vector_list_lengths(pca_projection_clusters, ctx.no_clusters, &vector_lengths_pca_clusters);

        #pragma omp parallel for schedule(dynamic, 1000)
        for (j = 0; j < ctx.samples->sample_count; j++) {
            /* iterate over all samples */

            VALUE_TYPE dist;
            uint64_t cluster_id, sample_id;
            struct sparse_vector pca_projection;
            pca_projection.nnz = 0;
            pca_projection.keys = NULL;
            pca_projection.values = NULL;

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
                        dist = lower_bound_euclid(vector_lengths_pca_clusters[cluster_id]
                                                  , vector_lengths_pca_samples[sample_id]);

                        if (dist >= ctx.cluster_distances[sample_id]) {
                            /* approximated distance is larger than current best distance. skip full distance calculation */
                            saved_calculations_cauchy += 1;
                            goto end;
                        }
                        if (prms->kmeans_algorithm_id == ALGORITHM_PCA_KMEANS) {
                            /* evaluate block vector approximation. */

                            dist = euclid_vector(pca_projection_samples[sample_id].keys
                                                 , pca_projection_samples[sample_id].values
                                                 , pca_projection_samples[sample_id].nnz
                                                 , pca_projection_clusters[cluster_id].keys
                                                 , pca_projection_clusters[cluster_id].values
                                                 , pca_projection_clusters[cluster_id].nnz
                                                 , vector_lengths_pca_samples[sample_id]
                                                 , vector_lengths_pca_clusters[cluster_id]);

                        } else {
                            if (pca_projection.keys == NULL) {
                                vector_matrix_dot(pca_projection_samples[sample_id].keys,
                                                  pca_projection_samples[sample_id].values,
                                                  pca_projection_samples[sample_id].nnz,
                                                  prms->ext_vects,
                                                  &pca_projection);
                            }

                            dist = euclid_vector(pca_projection.keys, pca_projection.values, pca_projection.nnz
                                                 , pca_projection_clusters[cluster_id].keys
                                                 , pca_projection_clusters[cluster_id].values
                                                 , pca_projection_clusters[cluster_id].nnz
                                                 , ctx.vector_lengths_samples[sample_id]
                                                 , ctx.vector_lengths_clusters[cluster_id]);
                        }

                        done_blockvector_calcs += 1;

                        if (dist >= ctx.cluster_distances[sample_id] && fabs(dist - ctx.cluster_distances[sample_id]) >= 1e-6) {
                            /* approximated distance is larger than current best distance. skip full distance calculation */
                            saved_calculations_pca += 1;
                            goto end;
                        }
                    }
                    /* printf("Approximated dist = %.4f - %.4f", dist, ctx.cluster_distances[sample_id]); */
                    /* if we reached this point we need to calculate a full euclidean distance */
                    dist = euclid_vector_list(ctx.samples, sample_id, ctx.cluster_vectors, cluster_id
                            , ctx.vector_lengths_samples, ctx.vector_lengths_clusters);
                    /* printf("actual dist = %.4f\n", dist); */
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
                free_null(pca_projection.keys);
                free_null(pca_projection.values);
            }
        }

        post_process_iteration(&ctx, prms);

        /* shift clusters to new position */
        calculate_shifted_clusters(&ctx);
        switch_to_shifted_clusters(&ctx);

        if (!disable_optimizations) {
            /* update only projections for cluster that shifted */
            update_dot_products(ctx.cluster_vectors,
                                ctx.no_clusters,
                                prms->ext_vects,
                                ctx.clusters_not_changed,
                                pca_projection_clusters);

            d_add_ilist(&(prms->tr), "iteration_pca_calcs", done_blockvector_calcs);
            d_add_ilist(&(prms->tr), "iteration_pca_calcs_success", saved_calculations_pca + saved_calculations_cauchy);

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
        if (prms->verbose) LOG_INFO("PCA statistics c:%" PRINTF_INT64_MODIFIER "u/b:%" PRINTF_INT64_MODIFIER "u/db:%" PRINTF_INT64_MODIFIER "u/pc:%" PRINTF_INT64_MODIFIER "u"
                , saved_calculations_cauchy
                , saved_calculations_pca
                , done_blockvector_calcs
                , saved_calculations_prev_cluster);
    }

    if (prms->verbose) LOG_INFO("total total_no_calcs = %" PRINTF_INT64_MODIFIER "u", ctx.total_no_calcs);

    resulting_clusters = create_result_clusters(prms, &ctx);

    /* cleanup all */
    if (!disable_optimizations) {
        if (prms->kmeans_algorithm_id == ALGORITHM_PCA_KMEANS) {
            free_vector_list(pca_projection_samples, samples->sample_count);
            free(vector_lengths_pca_samples);
            free(pca_projection_samples);
        }
        free_vector_list(pca_projection_clusters, ctx.no_clusters);
        free(pca_projection_clusters);
        free(vector_lengths_pca_clusters);
    }

    free_general_context(&ctx, prms);
    free_null(eligible_for_cluster_no_change_optimization);


    return resulting_clusters;
}
