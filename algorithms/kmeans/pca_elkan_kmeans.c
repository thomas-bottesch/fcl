#include "pca_elkan_kmeans.h"
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

#include "elkan_commons.h"

struct csr_matrix* pca_elkan_kmeans(struct csr_matrix* samples, struct kmeans_params *prms) {

    uint64_t i;
    uint64_t j;
    uint64_t k;
    struct sparse_vector* pca_projection_samples;  /* projection matrix of samples */
    struct sparse_vector* pca_projection_clusters; /* projection matrix of clusters */
    struct csr_matrix* resulting_clusters;
    uint32_t disable_optimizations;
    struct general_kmeans_context ctx;

    VALUE_TYPE* vector_lengths_pca_samples;
    VALUE_TYPE* vector_lengths_pca_clusters;

    char* bound_needs_update;                           /* bool per sample, 1 if a bound needs updating (rx)*/
    VALUE_TYPE** lb_samples_clusters;                   /* lower bounds for every sample to every cluster (lxc) */
    VALUE_TYPE** dist_clusters_clusters;                /* distance from, to every cluster (dcc) */
    VALUE_TYPE*  min_dist_cluster_clusters;             /* minimum distance between a cluster and all other clusters (sc) */
    VALUE_TYPE*  distance_clustersold_to_clustersnew;   /* distance between clusters before/after a shift */
    /* upper bounds from samples to clusters are stored in ctx.cluster_distances */

    initialize_general_context(prms, &ctx, samples);

	disable_optimizations = (prms->ext_vects == NULL || prms->kmeans_algorithm_id == ALGORITHM_ELKAN_KMEANS);
	
	if (disable_optimizations && prms->kmeans_algorithm_id == ALGORITHM_PCA_ELKAN_KMEANS) {
	    if (prms->verbose) LOG_ERROR("Unable to do pca_elkan_kmeans since no file_input_vectors was supplied. Doing regular elkan instead!");
	}
	
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
        /* track how many projection calculations were made / saved */
        uint64_t saved_calculations_pca;
        uint64_t done_pca_calcs;

        /* initialize data needed for the iteration */
        pre_process_iteration(&ctx);

	    if (!disable_optimizations) {
            /* reset all calculation counters */
            done_pca_calcs = 0;
            saved_calculations_pca = 0;

            free(vector_lengths_pca_clusters);
            calculate_vector_list_lengths(pca_projection_clusters, ctx.no_clusters, &vector_lengths_pca_clusters);
		}
		
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

						if (!disable_optimizations) {
                            dist = euclid_vector(pca_projection_samples[sample_id].keys
                                                 , pca_projection_samples[sample_id].values
                                                 , pca_projection_samples[sample_id].nnz
                                                 , pca_projection_clusters[cluster_id].keys
                                                 , pca_projection_clusters[cluster_id].values
                                                 , pca_projection_clusters[cluster_id].nnz
                                                 , vector_lengths_pca_samples[sample_id]
                                                 , vector_lengths_pca_clusters[cluster_id]);
                            done_pca_calcs += 1;

                            if (dist >= ctx.cluster_distances[sample_id]) {
                                /* tighten lower bound (if possible) */
                                if (dist > lb_samples_clusters[sample_id][cluster_id]) {
                                    lb_samples_clusters[sample_id][cluster_id] = dist;
                                }
                                saved_calculations_pca += 1;
                                continue;
                            }
						}
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

		if (!disable_optimizations) {
            /* update only projections for cluster that shifted */
            update_dot_products(ctx.cluster_vectors,
                                ctx.no_clusters,
                                prms->ext_vects,
                                ctx.clusters_not_changed,
                                pca_projection_clusters);

            d_add_ilist(&(prms->tr), "iteration_pca_calcs", done_pca_calcs);
            d_add_ilist(&(prms->tr), "iteration_pca_calcs_success",
                        saved_calculations_pca);
		}
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

        if (!disable_optimizations) {
            /* print projection statistics */
            if (prms->verbose) LOG_INFO("PCA statistics b:%" PRINTF_INT64_MODIFIER "u/db:%" PRINTF_INT64_MODIFIER "u"
                    , saved_calculations_pca
                    , done_pca_calcs);
        }
    }

    if (prms->verbose) LOG_INFO("total total_no_calcs = %" PRINTF_INT64_MODIFIER "u", ctx.total_no_calcs);

    resulting_clusters = create_result_clusters(prms, &ctx);

    /* cleanup all */
    free_general_context(&ctx, prms);
    if (!disable_optimizations) {
        free_vector_list(pca_projection_samples, samples->sample_count);
        free(vector_lengths_pca_samples);
        free(pca_projection_samples);

        free_vector_list(pca_projection_clusters, ctx.no_clusters);
        free(pca_projection_clusters);
        free(vector_lengths_pca_clusters);
	}
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
