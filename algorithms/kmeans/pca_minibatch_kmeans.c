#include "kmeans.h"
#include "kmeans_utils.h"
#include "minibatch_commons.h"

#include "../../utils/matrix/csr_matrix/csr_to_vector_list.h"
#include "../../utils/matrix/vector_list/vector_list_math.h"
#include "../../utils/matrix/csr_matrix/csr_math.h"
#include "../../utils/vector/common/common_vector_math.h"
#include "../../utils/vector/sparse/sparse_vector_math.h"
#include "../../utils/fcl_logging.h"

#include <unistd.h>
#include <float.h>
#include <math.h>

struct kmeans_result* pca_minibatch_kmeans(struct csr_matrix* samples
                                              , struct kmeans_params *prms) {


    uint32_t i;
    uint64_t j;
    uint64_t samples_per_batch;
    uint32_t max_not_improved_counter;
    uint32_t disable_optimizations;
	uint32_t* chosen_sample_map;
    struct sparse_vector* pca_projection_samples;  /* projection matrix of samples */
    struct sparse_vector* pca_projection_clusters; /* projection matrix of clusters */

	struct convergence_context conv_ctx;
	
    VALUE_TYPE* vector_lengths_pca_samples;
    VALUE_TYPE* vector_lengths_pca_clusters;
    struct kmeans_result* res;
    struct general_kmeans_context ctx;

    pca_projection_clusters = NULL;
    pca_projection_samples = NULL;

    initialize_general_context(prms, &ctx, samples);
    conv_ctx.initialized = 0;
    max_not_improved_counter = 20;

    /* if clusters_raw was filled (this happens in kmeans++) free it
     * since minibatch k-means uses a different strategy to fill the raw clusters
     */
    free_cluster_hashmaps(ctx.clusters_raw, ctx.no_clusters);

    /* reset cluster counts since minibatch kmeans handels them differently */
    for (i = 0; i < ctx.no_clusters; i++) ctx.cluster_counts[i] = 0;

    chosen_sample_map = NULL;
	/* samples_per_batch = ctx.samples->sample_count; */

    samples_per_batch = d_get_subint_default(&(prms->tr)
                                            , "additional_params", "samples_per_batch", ctx.samples->sample_count * 0.05);

    disable_optimizations = prms->ext_vects == NULL;
	
    if (!disable_optimizations) {
        /* create pca projections for the samples */
        pca_projection_samples = matrix_dot(samples, prms->ext_vects);
        calculate_vector_list_lengths(pca_projection_samples, samples->sample_count, &vector_lengths_pca_samples);

        /* create pca projections for the clusters */
        pca_projection_clusters = sparse_vectors_matrix_dot(ctx.cluster_vectors,
                                                            ctx.no_clusters,
                                                            prms->ext_vects);

        vector_lengths_pca_clusters = NULL;
    }

    create_chosen_sample_map(&chosen_sample_map, ctx.samples->sample_count, samples_per_batch, &(prms->seed));

    for (i = 0; i < prms->iteration_limit && !ctx.converged && !prms->stop; i++) {
        /* track how many blockvector calculations were made / saved */
        uint64_t saved_calculations_pca;
        uint64_t done_pca_calcs, saved_calculations_cauchy;
		
        /* reset all calculation counters */
        done_pca_calcs = 0;
        saved_calculations_cauchy = 0;
        saved_calculations_pca = 0;

        /* initialize data needed for the iteration */
        pre_process_iteration(&ctx);

        if (!disable_optimizations) {
            free(vector_lengths_pca_clusters);
            calculate_vector_list_lengths(pca_projection_clusters, ctx.no_clusters, &vector_lengths_pca_clusters);
        }

        #pragma omp parallel for schedule(dynamic, 1000)
        for (j = 0; j < ctx.samples->sample_count; j++) {
            /* iterate over all samples */

            VALUE_TYPE dist;
            uint64_t cluster_id, sample_id;

            if (!prms->stop && chosen_sample_map[j]) {
                sample_id = j;

                if (omp_get_thread_num() == 0) check_signals(&(prms->stop));

                for (cluster_id = 0; cluster_id < ctx.no_clusters; cluster_id++) {
                    /* iterate over all cluster centers */

                    if (!disable_optimizations) {
                        /* bv_minibatch_kmeans */

                        /* we already know the distance to the cluster from last iteration */
                        if (cluster_id == ctx.previous_cluster_assignments[sample_id]) continue;

                        /* evaluate cauchy approximation. fast but not good */
                        dist = lower_bound_euclid(ctx.vector_lengths_clusters[cluster_id]
                                                  , ctx.vector_lengths_samples[sample_id]);

                        if (dist >= ctx.cluster_distances[sample_id]) {
                            /* approximated distance is larger than current best distance. skip full distance calculation */
                            saved_calculations_cauchy += 1;
                            goto end;
                        }

                        dist = euclid_vector(pca_projection_samples[sample_id].keys
                                             , pca_projection_samples[sample_id].values
                                             , pca_projection_samples[sample_id].nnz
                                             , pca_projection_clusters[cluster_id].keys
                                             , pca_projection_clusters[cluster_id].values
                                             , pca_projection_clusters[cluster_id].nnz
                                             , vector_lengths_pca_samples[sample_id]
                                             , vector_lengths_pca_clusters[cluster_id]);

                        done_pca_calcs += 1;

                        if (dist >= ctx.cluster_distances[sample_id] && fabs(dist - ctx.cluster_distances[sample_id]) >= 1e-6) {
                            /* approximated distance is larger than current best distance. skip full distance calculation */
                            saved_calculations_pca += 1;
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
        }

        check_signals(&(prms->stop));
        post_process_iteration_minibatch(&ctx
                                        , chosen_sample_map
                                        , max_not_improved_counter
                                        , &conv_ctx);

        /* shift clusters to new position */
        calculate_shifted_clusters_minibatch_kmeans(&ctx, chosen_sample_map);
        /* calculate_shifted_clusters(&ctx); */
        switch_to_shifted_clusters(&ctx);

        create_chosen_sample_map(&chosen_sample_map, ctx.samples->sample_count, samples_per_batch, &(prms->seed));

        if (!disable_optimizations) {
            /* update only projections for cluster that shifted */
            update_dot_products(ctx.cluster_vectors,
                                ctx.no_clusters,
                                prms->ext_vects,
                                ctx.clusters_not_changed,
                                pca_projection_clusters);

            d_add_ilist(&(prms->tr), "iteration_pca_calcs", done_pca_calcs);
            d_add_ilist(&(prms->tr), "iteration_pca_calcs_success", saved_calculations_pca + saved_calculations_cauchy);
        }

        #pragma omp parallel for
        for (j = 0; j < ctx.samples->sample_count; j++) {
            /* iterate over all chosen samples for the next iteration and
             * update their distance to their current cluster
             */

            if (chosen_sample_map[j]) {

                ctx.cluster_distances[j]
                  = euclid_vector_list(ctx.samples, j
                          , ctx.cluster_vectors, ctx.cluster_assignments[j]
                          , ctx.vector_lengths_samples
                          , ctx.vector_lengths_clusters);

                /*#pragma omp critical*/
                ctx.done_calculations += 1;
                ctx.total_no_calcs += 1;
            }
        }

        print_iteration_summary(&ctx, prms, i);

        if (!disable_optimizations) {
            /* print projection statistics */
            if (prms->verbose) LOG_INFO("PCA statistics b:%" PRINTF_INT64_MODIFIER "u/db:%" PRINTF_INT64_MODIFIER "u"
                    , saved_calculations_pca
                    , done_pca_calcs);
        }
    }

    if (prms->verbose) LOG_INFO("total total_no_calcs = %" PRINTF_INT64_MODIFIER "u", ctx.total_no_calcs);

    res = create_kmeans_result(prms, &ctx);

    /* cleanup all */
    if (!disable_optimizations) {
        free_vector_list(pca_projection_samples, samples->sample_count);
        free(vector_lengths_pca_samples);
        free(pca_projection_samples);

        free_vector_list(pca_projection_clusters, ctx.no_clusters);
        free(pca_projection_clusters);
        free(vector_lengths_pca_clusters);
    }
    free_null(chosen_sample_map);
    free_general_context(&ctx, prms);
    return res;
}
