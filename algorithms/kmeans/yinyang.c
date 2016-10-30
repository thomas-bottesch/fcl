#include "yinyang.h"
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

struct csr_matrix* yinyang_kmeans(struct csr_matrix* samples, struct kmeans_params *prms) {

    uint32_t i;
    uint64_t j;
    uint64_t block_vectors_dim;
    uint64_t no_groups;
    uint32_t disable_optimizations;
    uint64_t keys_per_block;
    VALUE_TYPE desired_bv_annz;         /* desired size of the block vectors */

    uint64_t *cluster_to_group;

    struct csr_matrix block_vectors_samples;
    struct sparse_vector* block_vectors_clusters; /* block vector matrix of clusters */
    struct general_kmeans_context ctx;
    struct csr_matrix* resulting_clusters;

    VALUE_TYPE *distance_clustersold_to_clustersnew;

    struct group* groups;

    VALUE_TYPE *group_max_drift;
    VALUE_TYPE **lower_bounds;

    disable_optimizations = prms->kmeans_algorithm_id == ALGORITHM_YINYANG;
    initialize_general_context(prms, &ctx, samples);

    desired_bv_annz = d_get_subfloat_default(&(prms->tr)
                                            , "additional_params", "bv_annz", 0.3);
    block_vectors_dim = 0;
    keys_per_block = 0;

    if (!disable_optimizations) {
        initialize_csr_matrix_zero(&block_vectors_samples);

        if (prms->kmeans_algorithm_id == ALGORITHM_FAST_YINYANG) {
        /* search for a suitable size of the block vectors for the input samples and create them */
        search_samples_block_vectors(prms, ctx.samples, desired_bv_annz
                                     , &block_vectors_samples
                                     , &block_vectors_dim);
        }

        if (prms->kmeans_algorithm_id == ALGORITHM_FAST_YINYANG_ONDEMAND) {
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

    distance_clustersold_to_clustersnew = (VALUE_TYPE*) calloc(ctx.no_clusters, sizeof(VALUE_TYPE));

    /* no_groups is set to no_clusters / 10 as suggested in the yinyang paper */
    no_groups = ctx.no_clusters / 10;
    if (no_groups == 0) no_groups = 1;

    /* create yinyang cluster groups by doing 5 k-means iterations on the clusters */
    create_kmeans_cluster_groups(ctx.cluster_vectors
                                , ctx.no_clusters
                                , ctx.samples->sample_count
                                , &groups, &no_groups);


    group_max_drift = (VALUE_TYPE*) calloc(no_groups, sizeof(VALUE_TYPE));
    lower_bounds = (VALUE_TYPE**) calloc(ctx.samples->sample_count, sizeof(VALUE_TYPE*));

    for (i = 0; i < ctx.samples->sample_count; i++) {
        lower_bounds[i] = (VALUE_TYPE*) calloc(no_groups, sizeof(VALUE_TYPE));
    }

    cluster_to_group = (uint64_t*) calloc(ctx.no_clusters, sizeof(uint64_t));

    for (i = 0; i < no_groups; i++) {
        for (j = 0; j < groups[i].no_clusters; j++) {
            cluster_to_group[groups[i].clusters[j]] = i;
        }
    }

    for (i = 0; i < prms->iteration_limit && !ctx.converged && !prms->stop; i++) {
        uint64_t saved_calculations_prev_cluster, saved_calculations_bv;
        uint64_t saved_calculations_global, saved_calculations_local;
        uint64_t done_blockvector_calcs;
        uint64_t groups_not_skipped;

        done_blockvector_calcs = 0;
        saved_calculations_bv = 0;
        saved_calculations_global = 0;
        saved_calculations_local = 0;
        saved_calculations_prev_cluster = 0;
        groups_not_skipped = 0;

        /* initialize data needed for the iteration */
        pre_process_iteration(&ctx);

        if (i == 0) {
            /* first iteration is done with regular kmeans to find the upper and lower bounds */
            uint64_t sample_id, l;

            /* do one regular kmeans step to initialize bounds */
            #pragma omp parallel for schedule(dynamic, 1000) private(l)
            for (sample_id = 0; sample_id < ctx.samples->sample_count; sample_id++) {
                uint64_t cluster_id;
                VALUE_TYPE dist;
                uint32_t is_first_assignment;
                struct sparse_vector bv;
                bv.nnz = 0;
                bv.keys = NULL;
                bv.values = NULL;
                is_first_assignment = 0;

                if (omp_get_thread_num() == 0) check_signals(&(prms->stop));

                if (!prms->stop) {
                    for (l = 0; l < no_groups; l++) {
                        lower_bounds[sample_id][l] = DBL_MAX;
                    }

                    for (cluster_id = 0; cluster_id < ctx.no_clusters; cluster_id++) {
                        if (!disable_optimizations) {
                            /* block vector optimizations */

                            /* check if sqrt( ||s||² + ||c||² - 2*< s_B, c_B > ) >= ctx.cluster_distances[sample_id] */
                            if (prms->kmeans_algorithm_id == ALGORITHM_FAST_YINYANG) {
                                /* evaluate block vector approximation. */
                                dist = euclid_vector_list(&block_vectors_samples, sample_id
                                              , block_vectors_clusters, cluster_id
                                              , ctx.vector_lengths_samples
                                              , ctx.vector_lengths_clusters);
                            } else {
                                /* kmeans_algorithm_id == ALGORITHM_FAST_YINYANG_ONDEMAND */
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

                            /* we do this fabs to not run into numeric errors */
                            if (dist >= ctx.cluster_distances[sample_id] && fabs(dist - ctx.cluster_distances[sample_id]) >= 1e-6) {
                                saved_calculations_bv += 1;
                                goto end_cluster_init;
                            }

                        }

                        dist = euclid_vector_list(samples, sample_id, ctx.cluster_vectors, cluster_id
                                , ctx.vector_lengths_samples, ctx.vector_lengths_clusters);

                        /*#pragma omp critical*/
                        ctx.done_calculations += 1;

                        if (dist < ctx.cluster_distances[sample_id]) {
                            if (is_first_assignment) {
                                is_first_assignment = 0;
                            } else {
                                lower_bounds[sample_id][cluster_to_group[ctx.cluster_assignments[sample_id]]] = ctx.cluster_distances[sample_id];
                            }

                            ctx.cluster_distances[sample_id] = dist;
                            ctx.cluster_assignments[sample_id] = cluster_id;
                        } else {
                            end_cluster_init:;
                            if (dist < lower_bounds[sample_id][cluster_to_group[cluster_id]]) {
                                lower_bounds[sample_id][cluster_to_group[cluster_id]] = dist;
                            }
                        }
                    }
                }
                if (!disable_optimizations) {
                    free_null(bv.keys);
                    free_null(bv.values);
                }
            }
        } else {

            #pragma omp parallel for schedule(dynamic, 1000)
            for (j = 0; j < ctx.samples->sample_count; j++) {
                VALUE_TYPE dist;
                uint64_t cluster_id, sample_id, l;
                VALUE_TYPE *temp_lower_bounds;
                VALUE_TYPE global_lower_bound;
                VALUE_TYPE *should_group_be_updated;
                struct sparse_vector bv;
                bv.nnz = 0;
                bv.keys = NULL;
                bv.values = NULL;

                sample_id = j;

                if (omp_get_thread_num() == 0) check_signals(&(prms->stop));

                if (!prms->stop) {
                    /* update upper bound of this sample with drift of assigned cluster */
                    ctx.cluster_distances[sample_id] = ctx.cluster_distances[sample_id] + distance_clustersold_to_clustersnew[ctx.cluster_assignments[sample_id]];

                    temp_lower_bounds = (VALUE_TYPE*) calloc(no_groups, sizeof(VALUE_TYPE));
                    should_group_be_updated = (VALUE_TYPE*) calloc(no_groups, sizeof(VALUE_TYPE));

                    global_lower_bound = DBL_MAX;
                    for (l = 0; l < no_groups; l++) {
                        temp_lower_bounds[l] = lower_bounds[sample_id][l];
                        lower_bounds[sample_id][l] = lower_bounds[sample_id][l] - group_max_drift[l];
                        if (global_lower_bound > lower_bounds[sample_id][l]) global_lower_bound = lower_bounds[sample_id][l];
                    }

                    /* check if the global lower bound is already bigger than the current upper bound */
                    if (global_lower_bound >= ctx.cluster_distances[sample_id]) {
                        saved_calculations_global += ctx.no_clusters;
                        goto end;
                    }

                    /* tighten the upper bound by calculating the actual distance to the current closest cluster */
                    ctx.cluster_distances[sample_id]
                       = euclid_vector_list(samples, sample_id, ctx.cluster_vectors, ctx.cluster_assignments[sample_id]
                                , ctx.vector_lengths_samples, ctx.vector_lengths_clusters);

                    /*#pragma omp critical*/
                    ctx.done_calculations += 1;

                    /* recheck if the global lower bound is now bigger than the current upper bound */
                    if (global_lower_bound >= ctx.cluster_distances[sample_id]) {
                        saved_calculations_global += ctx.no_clusters - 1;
                        goto end;
                    }

                    for (l = 0; l < no_groups; l++) {
                        if (lower_bounds[sample_id][l] < ctx.cluster_distances[sample_id]) {
                            should_group_be_updated[l] = 1;
                            groups_not_skipped += 1;
                            lower_bounds[sample_id][l] = DBL_MAX;
                        }
                    }

                    for (cluster_id = 0; cluster_id < ctx.no_clusters; cluster_id++) {
                        if (!should_group_be_updated[cluster_to_group[cluster_id]]) {
                            saved_calculations_prev_cluster++;
                            continue;
                        }
                        if (ctx.cluster_counts[cluster_id] == 0 || cluster_id == ctx.previous_cluster_assignments[sample_id]) continue;

                        if (lower_bounds[sample_id][cluster_to_group[cluster_id]] < temp_lower_bounds[cluster_to_group[cluster_id]] - distance_clustersold_to_clustersnew[cluster_id]) {
                            dist = lower_bounds[sample_id][cluster_to_group[cluster_id]];
                            saved_calculations_local += 1;
                            goto end_cluster;
                        }

                        if (!disable_optimizations) {
                            if (i < 15) {
                                /* block vector optimizations */
                                /* check if sqrt( ||s||² + ||c||² - 2*< s_B, c_B > ) >= ctx.cluster_distances[sample_id] */
                                if (prms->kmeans_algorithm_id == ALGORITHM_FAST_YINYANG) {
                                    /* evaluate block vector approximation. */
                                    dist = euclid_vector_list(&block_vectors_samples, sample_id
                                                  , block_vectors_clusters, cluster_id
                                                  , ctx.vector_lengths_samples
                                                  , ctx.vector_lengths_clusters);
                                } else {
                                    /* kmeans_algorithm_id == ALGORITHM_FAST_YINYANG_ONDEMAND */
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
                                    saved_calculations_bv += 1;
                                    goto end_cluster;
                                }
                            }
                        }

                        dist = euclid_vector_list(samples, sample_id, ctx.cluster_vectors, cluster_id
                                , ctx.vector_lengths_samples, ctx.vector_lengths_clusters);

                        /*#pragma omp critical*/
                        ctx.done_calculations += 1;

                        if (dist < ctx.cluster_distances[sample_id]) {
                            lower_bounds[sample_id][cluster_to_group[ctx.cluster_assignments[sample_id]]] = ctx.cluster_distances[sample_id];
                            ctx.cluster_distances[sample_id] = dist;
                            ctx.cluster_assignments[sample_id] = cluster_id;
                        } else {
                            end_cluster:;
                            if (dist < lower_bounds[sample_id][cluster_to_group[cluster_id]]) {
                                lower_bounds[sample_id][cluster_to_group[cluster_id]] = dist;
                            }
                        }
                    }

                    end:;
                    free(should_group_be_updated);
                    free(temp_lower_bounds);
                }
                if (!disable_optimizations) {
                    free_null(bv.keys);
                    free_null(bv.values);
                }
            } /* block iterate over samples */
        } /* block is first iteration */

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

        /* ------------ calculate maximum drift for every group ------------- */
        {
            uint64_t *clusters;
            uint64_t n_clusters, l, k;
            VALUE_TYPE drift;

            for (l = 0; l < no_groups; l++) {
                clusters = groups[l].clusters;
                n_clusters = groups[l].no_clusters;
                group_max_drift[l] = 0;
                for (k = 0; k < n_clusters; k++) {
                    drift = distance_clustersold_to_clustersnew[clusters[k]];
                    if (group_max_drift[l] < drift) group_max_drift[l] = drift;
                }
            }
        }


        if (!disable_optimizations) {
            /* update only block vectors for cluster that shifted */
            update_changed_blockvectors(ctx.cluster_vectors
                                        , block_vectors_dim
                                        , ctx.no_clusters
                                        , ctx.samples->dim
                                        , ctx.clusters_not_changed
                                        , block_vectors_clusters);

            d_add_ilist(&(prms->tr), "iteration_bv_calcs", done_blockvector_calcs);
            d_add_ilist(&(prms->tr), "iteration_bv_calcs_success", saved_calculations_bv);
        }

        print_iteration_summary(&ctx, prms, i);

        /* print block vector and yinyang statistics */
        if (prms->verbose) LOG_INFO("statistics [BV] b:%" PRINTF_INT64_MODIFIER "u/db:%" PRINTF_INT64_MODIFIER "u [YY] grp_not_skip=%" PRINTF_INT64_MODIFIER "u/pc:%" PRINTF_INT64_MODIFIER "u/g=%" PRINTF_INT64_MODIFIER "u/l=%" PRINTF_INT64_MODIFIER "u"
                , saved_calculations_bv
                , done_blockvector_calcs
                , groups_not_skipped
                , saved_calculations_prev_cluster
                , saved_calculations_global
                , saved_calculations_local);

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
    free_null(distance_clustersold_to_clustersnew);
    free_null(group_max_drift);

    for (i = 0; i < ctx.samples->sample_count; i++) {
        free_null(lower_bounds[i]);
    }

    for (i = 0; i < no_groups; i++) {
        free_null(groups[i].clusters);
    }

    free_null(groups);
    free_null(lower_bounds);
    free_null(cluster_to_group);

    return resulting_clusters;
}
