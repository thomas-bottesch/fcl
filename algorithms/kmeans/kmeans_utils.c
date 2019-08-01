#include "kmeans_utils.h"
#include <math.h>
#include <float.h>
#include "kmeans.h"

#include "../../utils/fcl_logging.h"
#include "../../utils/fcl_time.h"
#include "../../utils/matrix/csr_matrix/csr_to_vector_list.h"
#include "../../utils/matrix/csr_matrix/csr_math.h"
#include "../../utils/matrix/vector_list/vector_list_math.h"
#include "../../utils/matrix/csr_matrix/csr_assign.h"
#include "../../utils/matrix/vector_list/vector_list_to_csr.h"
#include "../../utils/matrix/vector_list/vector_list.h"
#include "../../utils/vector/sparse/sparse_vector_math.h"

#define UPDATE_TYPE_KMEANS           UINT32_C(0)
#define UPDATE_TYPE_MINIBATCH_KMEANS UINT32_C(1)

typedef void (*kmeans_init_function) (struct general_kmeans_context* ctx
        											, struct kmeans_params *prms);

typedef void (*kmeans_preinit_function) (struct csr_matrix *mtrx, struct kmeans_params *prms);

kmeans_init_function KMEANS_INIT_FUNCTIONS[NO_KMEANS_INITS] \
                                      = {initialize_kmeans_random,
                                         initialize_kmeans_pp,
                                         initialize_kmeans_init_params};

kmeans_preinit_function KMEANS_PREINIT_FUNCTIONS[NO_KMEANS_INITS] \
                                                  = {NULL,
                                                     NULL,
                                                     preinitialize_kmeans_init_params};

void get_kmeanspp_assigns(struct csr_matrix *mtrx
                          , struct csr_matrix *blockvectors_mtrx
                          , struct sparse_vector* pca_projection_samples
                          , VALUE_TYPE *sparse_vector_lengths
                          , VALUE_TYPE *pca_sparse_vector_lengths
                          , uint64_t no_clusters
                          , uint64_t *cluster_counts
                          , uint64_t *cluster_assignments
                          , VALUE_TYPE *cluster_distances
                          , uint32_t* seed
                          , uint64_t use_triangle_inequality
                          , uint64_t *initial_cluster_samples
                          , uint32_t verbose
                          , struct cdict* tr
                          , uint32_t* stop);

void free_general_context(struct general_kmeans_context* ctx
                          , struct kmeans_params *prms) {

    free_vector_list(ctx->cluster_vectors, ctx->no_clusters);
    free_null(ctx->cluster_vectors);
    free_cluster_hashmaps(ctx->clusters_raw, ctx->no_clusters);
    free_null(ctx->clusters_raw);
    free_null(ctx->cluster_distances);
    free_null(ctx->cluster_assignments);
    free_null(ctx->initial_cluster_samples);
    free_null(ctx->cluster_counts);
    free_null(ctx->vector_lengths_samples);
    free_null(ctx->vector_lengths_clusters);
    free_null(ctx->clusters_not_changed);
    free_null(ctx->was_assigned);
    free_null(ctx->previous_cluster_assignments);
}

void free_kmeans_result(struct kmeans_result* res) {
    if (res->clusters != NULL) {
        free_csr_matrix(res->clusters);
        free_null(res->clusters);
    }

    free_init_params(res->initprms);
    free_null(res->initprms);
    free_null(res);
}

void initialize_kmeans_random(struct general_kmeans_context* ctx,
                              struct kmeans_params *prms) {
    uint64_t i;

    /* use random samples as clusters */
    create_vector_list_random(ctx->samples
                                     , ctx->cluster_vectors
                                     , ctx->no_clusters
                                     , &(prms->seed)
                                     , ctx->initial_cluster_samples);

    /* assign a random cluster to every sample */
    for (i = 0; i < ctx->samples->sample_count; i++) {
        ctx->cluster_assignments[i] = i % ctx->no_clusters;
    }
}

void preinitialize_kmeans_init_params(struct csr_matrix *samples,
                                      struct kmeans_params *prms) {
    uint64_t i;
    uint32_t no_clusters;

    no_clusters = 0;

    if (prms->initprms == NULL) {
        if (prms->verbose) LOG_ERROR("Initprms are empty");
        goto error_invalid_init_data;
    }

    if (prms->initprms->assignments == NULL) {
        if (prms->verbose) LOG_ERROR("Init assignments are empty");
        goto error_invalid_init_data;
    }

    if (prms->initprms->len_assignments != samples->sample_count) {
        if (prms->verbose) LOG_ERROR("Init assignments have invalid length %" PRINTF_INT64_MODIFIER "u != %" PRINTF_INT64_MODIFIER "u", prms->initprms->len_assignments, samples->sample_count);
        goto error_invalid_init_data;
    }

    for (i = 0; i < samples->sample_count; i++) {
        if (prms->initprms->assignments[i] > no_clusters) {
            no_clusters = prms->initprms->assignments[i];
        }
    }
    no_clusters += 1;

    if (no_clusters > prms->initprms->len_initial_cluster_samples) {
        if (prms->verbose) LOG_ERROR("no_clusters > len_initial_cluster_samples");
        goto error_invalid_init_data;
    }

    if (prms->initprms->len_initial_cluster_samples > no_clusters) {
        no_clusters = prms->initprms->len_initial_cluster_samples;
    }

    for (i = 0; i < prms->initprms->len_initial_cluster_samples; i++) {
        if (prms->initprms->initial_cluster_samples[i] >= samples->sample_count) {
            if (prms->verbose) LOG_ERROR("prms->initprms->initial_cluster_samples[i] >= samples->sample_count");
            goto error_invalid_init_data;
        }
    }



    goto init_data_correct;

error_invalid_init_data:
    if (prms->verbose) LOG_ERROR("Invalid assignment_list data. Using random init instead with k=10.");
    prms->init_id = KMEANS_INIT_RANDOM;
    prms->no_clusters = 10;
    return;
init_data_correct:
    prms->no_clusters = no_clusters;
}

void initialize_kmeans_init_params(struct general_kmeans_context* ctx,
                                   struct kmeans_params *prms) {
    uint64_t i;
    KEY_TYPE *keys;
    VALUE_TYPE *values;
    uint64_t nnz;

    for (i = 0; i < ctx->samples->sample_count; i++) {
        ctx->cluster_assignments[i] = prms->initprms->assignments[i];
        ctx->cluster_counts[ctx->cluster_assignments[i]] += 1;
    }

    for (i = 0; i < ctx->samples->sample_count; i++) {
        keys = ctx->samples->keys + ctx->samples->pointers[i];
        values = ctx->samples->values + ctx->samples->pointers[i];
        nnz = ctx->samples->pointers[i + 1] - ctx->samples->pointers[i];
        add_sample_to_hashmap(ctx->clusters_raw, keys, values, nnz, ctx->cluster_assignments[i]);
        ctx->was_assigned[i] = 1;
    }

    for (i = 0; i < prms->no_clusters; i++) {
        HASH_SORT(ctx->clusters_raw[i], id_sort);
    }

    create_vector_list_from_hashmap(ctx->clusters_raw
                                        , ctx->cluster_counts
                                        , ctx->cluster_vectors
                                        , ctx->no_clusters);

    for (i = 0; i < prms->initprms->len_initial_cluster_samples; i++) {
        ctx->initial_cluster_samples[i] = prms->initprms->initial_cluster_samples[i];
    }

}

void initialize_kmeans_pp(struct general_kmeans_context* ctx,
                              struct kmeans_params *prms) {

    uint64_t i;
    KEY_TYPE *keys;
    VALUE_TYPE *values;
    uint64_t nnz;
    uint64_t use_triangle_inequality;

    /* Block vector variables */
    uint64_t block_vectors_dim;         /* size of block vectors */
    VALUE_TYPE desired_bv_annz;         /* desired size of the block vectors */
    struct csr_matrix block_vectors_samples;  /* block vector matrix of samples */

    /* PCA variables */
    uint64_t use_pca;                 /* desired size of the block vectors */
    VALUE_TYPE* vector_lengths_pca_samples;
    struct sparse_vector* pca_projection_samples;

    pca_projection_samples = NULL;
    vector_lengths_pca_samples = NULL;
    block_vectors_samples.sample_count = 0;
    desired_bv_annz = d_get_subfloat_default(&(prms->tr)
                                            , "additional_params", "kmpp_bv_annz", 0);

    use_triangle_inequality = d_get_subint_default(&(prms->tr)
                            , "additional_params", "kmpp_use_triangle_inequality", 0);

    use_pca = d_get_subint_default(&(prms->tr)
                            , "additional_params", "kmpp_use_pca", 0);

    if (use_pca && prms->ext_vects != NULL) {
        if (prms->verbose) LOG_INFO("kmeans++ pca activated");
        pca_projection_samples = matrix_dot(ctx->samples, prms->ext_vects);
        calculate_vector_list_lengths(pca_projection_samples, ctx->samples->sample_count, &vector_lengths_pca_samples);
    }

    if (desired_bv_annz > 0) {
        if (prms->verbose) LOG_INFO("kmeans++ block vectors activated with annz: %.3f", desired_bv_annz);
        determine_block_vectors_for_matrix(ctx->samples
                                           , desired_bv_annz
                                           , &block_vectors_samples
                                           , &block_vectors_dim);
        if (prms->verbose) LOG_INFO("kmeans++ done getting block vector matrix");
    }

    get_kmeanspp_assigns(ctx->samples
                         , &block_vectors_samples
                         , pca_projection_samples
                         , ctx->vector_lengths_samples
                         , vector_lengths_pca_samples
                         , prms->no_clusters
                         , ctx->cluster_counts
                         , ctx->cluster_assignments
                         , ctx->cluster_distances
                         , &(prms->seed)
                         , use_triangle_inequality
                         , ctx->initial_cluster_samples
                         , prms->verbose
                         , prms->tr
                         , &(prms->stop));

    if (desired_bv_annz > 0) {
        free_csr_matrix(&block_vectors_samples);
    }
    if (use_pca && prms->ext_vects != NULL) {
        free_vector_list(pca_projection_samples, ctx->samples->sample_count);
        free(vector_lengths_pca_samples);
        free(pca_projection_samples);
    }
    for (i = 0; i < ctx->samples->sample_count; i++) {
        keys = ctx->samples->keys + ctx->samples->pointers[i];
        values = ctx->samples->values + ctx->samples->pointers[i];
        nnz = ctx->samples->pointers[i + 1] - ctx->samples->pointers[i];
        add_sample_to_hashmap(ctx->clusters_raw, keys, values, nnz, ctx->cluster_assignments[i]);
        ctx->was_assigned[i] = 1;
    }

    for (i = 0; i < prms->no_clusters; i++) {
        HASH_SORT(ctx->clusters_raw[i], id_sort);
    }

    create_vector_list_from_hashmap(ctx->clusters_raw
                                        , ctx->cluster_counts
                                        , ctx->cluster_vectors
                                        , ctx->no_clusters);
}

void get_kmeanspp_assigns(struct csr_matrix *mtrx
                          , struct csr_matrix *blockvectors_mtrx
                          , struct sparse_vector* pca_projection_samples
                          , VALUE_TYPE *sparse_vector_lengths
                          , VALUE_TYPE *pca_sparse_vector_lengths
                          , uint64_t no_clusters
                          , uint64_t *cluster_counts
                          , uint64_t *cluster_assignments
                          , VALUE_TYPE *cluster_distances
                          , uint32_t* seed
                          , uint64_t use_triangle_inequality
                          , uint64_t *initial_cluster_samples
                          , uint32_t verbose
                          , struct cdict* tr
                          , uint32_t* stop) {

    uint64_t  no_clusters_so_far, i, j, calcs_skipped_tr, calcs_skipped_bv, calcs_skipped_pca, calcs_skipped_is_cluster;
    VALUE_TYPE rand_max;
    VALUE_TYPE approximated_full_distance_calcs_bv;
    VALUE_TYPE approximated_full_distance_calcs_pca;
    uint64_t mtrx_annz;
    uint64_t calcs_needed;
    uint64_t *is_cluster;
    VALUE_TYPE min_distances_cluster_new_cluster;
    time_t start;
    no_clusters_so_far = 0;
    calcs_needed = 0;
    rand_max = RAND_MAX;
    approximated_full_distance_calcs_bv = 0;
    approximated_full_distance_calcs_pca = 0;
    calcs_skipped_tr = 0;
    calcs_skipped_bv = 0;
    calcs_skipped_pca = 0;
    calcs_skipped_is_cluster = 0;
    start = time(NULL);

    is_cluster = (uint64_t*) calloc(mtrx->sample_count, sizeof(uint64_t));

    /* choose the first sample randomly from all samples */
    initial_cluster_samples[no_clusters_so_far] = rand_r(seed) % mtrx->sample_count;
    no_clusters_so_far += 1;

    /* initialize all distances with infinity */
    for (i = 0; i < mtrx->sample_count; i++) {
        cluster_distances[i] = DBL_MAX;
    }

    min_distances_cluster_new_cluster = 0;

    while (no_clusters_so_far <= no_clusters  && !(*stop)) {
        uint64_t cluster_id;
        VALUE_TYPE sum;
        if (no_clusters_so_far % 500 == 0) {
            if (verbose) LOG_INFO("kmeans++ chosen_clusters so far: %" PRINTF_INT64_MODIFIER "u (%d secs).. %" PRINTF_INT64_MODIFIER "u %" PRINTF_INT64_MODIFIER "u", no_clusters_so_far, (int) (time(NULL) - start), calcs_skipped_tr, calcs_skipped_bv);
            start = time(NULL);
        }

        /* no need to iterate over all clusters, only the recently added cluster is new info */
        cluster_id = initial_cluster_samples[no_clusters_so_far - 1];

        #pragma omp parallel for schedule(dynamic, 1000)
        for (i = 0; i < mtrx->sample_count; i++) {
            uint64_t sample_id;
            VALUE_TYPE dist;
            sample_id = i;

            if (omp_get_thread_num() == 0) check_signals(stop);

            if (!(*stop)) {

                if (is_cluster[sample_id]) {
                    calcs_skipped_is_cluster += 1;
                    continue;
                }

                if (use_triangle_inequality
                    && (min_distances_cluster_new_cluster >= cluster_distances[sample_id])) {
                    /* triangle inequality d(closest_cluster, new_cluster) >= 2 * d(sample, closest_cluster)
                     * --> d(sample, new_cluster) >= d(sample, closest_cluster)
                     */
                    calcs_skipped_tr += 1;
                    continue;
                }

                if (blockvectors_mtrx->sample_count > 0) {

                    dist = euclid_vector(blockvectors_mtrx->keys + blockvectors_mtrx->pointers[cluster_id]
                                         , blockvectors_mtrx->values + blockvectors_mtrx->pointers[cluster_id]
                                         , blockvectors_mtrx->pointers[cluster_id + 1] - blockvectors_mtrx->pointers[cluster_id]
                                         , blockvectors_mtrx->keys + blockvectors_mtrx->pointers[sample_id]
                                         , blockvectors_mtrx->values + blockvectors_mtrx->pointers[sample_id]
                                         , blockvectors_mtrx->pointers[sample_id + 1] - blockvectors_mtrx->pointers[sample_id]
                                         , sparse_vector_lengths[cluster_id]
                                         , sparse_vector_lengths[sample_id]);

                    if (dist >= cluster_distances[sample_id]) {
                        calcs_skipped_bv += 1;
                        continue;
                    }
                }

                if (pca_projection_samples != NULL) {
                    dist = euclid_vector(pca_projection_samples[sample_id].keys
                                         , pca_projection_samples[sample_id].values
                                         , pca_projection_samples[sample_id].nnz
                                         , pca_projection_samples[cluster_id].keys
                                         , pca_projection_samples[cluster_id].values
                                         , pca_projection_samples[cluster_id].nnz
                                         , pca_sparse_vector_lengths[sample_id]
                                         , pca_sparse_vector_lengths[cluster_id]);
                    if (dist >= cluster_distances[sample_id]) {
                        calcs_skipped_pca += 1;
                        continue;
                    }
                }

                dist = euclid_vector(mtrx->keys + mtrx->pointers[cluster_id]
                                            , mtrx->values + mtrx->pointers[cluster_id]
                                            , mtrx->pointers[cluster_id + 1] - mtrx->pointers[cluster_id]
                                            , mtrx->keys + mtrx->pointers[sample_id]
                                            , mtrx->values + mtrx->pointers[sample_id]
                                            , mtrx->pointers[sample_id + 1] - mtrx->pointers[sample_id]
                                            , sparse_vector_lengths[cluster_id]
                                            , sparse_vector_lengths[sample_id]);
                calcs_needed += 1;
                #pragma omp critical
                if (dist < cluster_distances[sample_id]) {
                    if (no_clusters_so_far != 1) {
                        cluster_counts[cluster_assignments[sample_id]] -= 1;

                    }

                    cluster_distances[sample_id] = dist;
                    cluster_assignments[sample_id] = no_clusters_so_far - 1;
                    cluster_counts[cluster_assignments[sample_id]] += 1;
                }
            }
        }

        if (no_clusters_so_far == no_clusters) break;
        sum = 0;

        #pragma omp parallel for reduction(+:sum)
        for (j = 0; j < mtrx->sample_count; j++) {
            sum += cluster_distances[j];
        }

        /* find new cluster depending on the closest distances from every sample to the already chosen clusters */
        sum = (rand_r(seed) / rand_max) * sum;

        for (j = 0; j < mtrx->sample_count; j++) {
            sum -= cluster_distances[j];
            if (sum < 0) break;
        }

        if (j == mtrx->sample_count) {
            initial_cluster_samples[no_clusters_so_far] = rand_r(seed) % mtrx->sample_count;
        } else {
            initial_cluster_samples[no_clusters_so_far] = j;
        }

        is_cluster[initial_cluster_samples[no_clusters_so_far]] = 1;
        min_distances_cluster_new_cluster = cluster_distances[initial_cluster_samples[no_clusters_so_far]] / 2;

        no_clusters_so_far++;
    }

    if (verbose) LOG_INFO("kmeans++ finished with %" PRINTF_INT64_MODIFIER "u clusters", no_clusters_so_far);
    if (verbose) LOG_INFO("kmeans++ calcs_needed %" PRINTF_INT64_MODIFIER "u/%" PRINTF_INT64_MODIFIER "u"
                          , calcs_needed, (mtrx->sample_count * no_clusters) - calcs_skipped_is_cluster);

    mtrx_annz = mtrx->pointers[mtrx->sample_count] / mtrx->sample_count;

    d_add_subint(&tr, "kmeans++", "block_vectors_enabled", blockvectors_mtrx->sample_count > 0);
    if (blockvectors_mtrx->sample_count > 0) {
        VALUE_TYPE block_vectors_annz = blockvectors_mtrx->pointers[blockvectors_mtrx->sample_count] / blockvectors_mtrx->sample_count;

        d_add_subint(&tr, "kmeans++", "block_vectors_dim", blockvectors_mtrx->dim);
        d_add_subint(&tr, "kmeans++", "block_vectors_annz", block_vectors_annz);
        d_add_subfloat(&tr, "kmeans++", "block_vectors_relative_annz", block_vectors_annz / mtrx_annz);
        approximated_full_distance_calcs_bv = (block_vectors_annz / mtrx_annz) * (calcs_needed + calcs_skipped_bv);
        /* Use (block_vectors_annz / mtrx_annz) * (calcs_needed + calcs_skipped_bv)
         * to translate the number of block_vector euclidean distance calculations
         * into full euclidean distance calculations. Then add the approximated
         * full calculations and the done full calculations to get the overall
         * approximated full calculations.
         */
    }

    d_add_subint(&tr, "kmeans++", "pca_enabled", pca_projection_samples != NULL);
    if (pca_projection_samples != NULL) {
        VALUE_TYPE pca_annz;
        KEY_TYPE max_sample_key;
        uint64_t pca_dim = 0;
        pca_annz = 0;
        for (i = 0; i < mtrx->sample_count; i++) {
            pca_annz += pca_projection_samples[i].nnz;
            if (pca_projection_samples[i].nnz > 0) {
                max_sample_key = pca_projection_samples[i].keys[pca_projection_samples[i].nnz - 1];
                if (max_sample_key > pca_dim) {
                    pca_dim = max_sample_key;
                }
            }
        }
        pca_dim += 1;
        pca_annz = pca_annz / mtrx->sample_count;

        d_add_subint(&tr, "kmeans++", "pca_dim", pca_dim);
        d_add_subint(&tr, "kmeans++", "pca_annz", pca_annz);
        d_add_subfloat(&tr, "kmeans++", "pca_relative_annz", pca_annz / mtrx_annz);
        approximated_full_distance_calcs_pca = (pca_annz / mtrx_annz) * (calcs_needed + calcs_skipped_pca);
    }

    if (pca_projection_samples != NULL || blockvectors_mtrx->sample_count > 0) {
        d_add_subint(&tr, "kmeans++", "approximated_full_distance_calcs"
                     , (approximated_full_distance_calcs_bv + approximated_full_distance_calcs_pca + calcs_needed));
    }

    d_add_subint(&tr, "kmeans++", "calculations_needed", calcs_needed);
    d_add_subint(&tr, "kmeans++", "calculations_needed_naive", (mtrx->sample_count * no_clusters) - calcs_skipped_is_cluster);

    free(is_cluster);
}

void pre_process_iteration(struct general_kmeans_context* ctx) {

    /* free old previous_cluster_assignments */
    free_null(ctx->previous_cluster_assignments);

    /* copy cluster_assignments before iteration to previous_cluster_assignments */
    ctx->previous_cluster_assignments = (uint64_t*) calloc(ctx->samples->sample_count, sizeof(uint64_t));
    memcpy(ctx->previous_cluster_assignments, ctx->cluster_assignments, ctx->samples->sample_count * sizeof(uint64_t) );

    /* reset all calculation counters */
    ctx->done_calculations = 0;
    ctx->no_changes = 0;

    if (ctx->track_time) ctx->duration_all_calcs = clock();

    gettimeofday(&(ctx->tm_start_iteration), NULL);
    gettimeofday(&(ctx->durations), NULL);
}

uint32_t batch_convergence(uint64_t no_samples
                           , uint64_t samples_per_batch
                           , VALUE_TYPE summed_batch_wcssd
                           , uint32_t max_not_improved_counter
                           , struct convergence_context* conv_ctx) {

    VALUE_TYPE ewa_wcssd, ewa_wcssd_min, alpha;
    uint32_t not_improved_counter;
    summed_batch_wcssd /= samples_per_batch;

    /*
     * Compute an Exponentially Weighted Average of the batch wcssd
     */
    if (!conv_ctx->initialized) {
        ewa_wcssd = summed_batch_wcssd;
        ewa_wcssd_min = ewa_wcssd;
        not_improved_counter = 0;
        conv_ctx->initialized = 1;
    } else {
        ewa_wcssd = conv_ctx->ewa_wcssd;
        ewa_wcssd_min = conv_ctx->ewa_wcssd_min;
        not_improved_counter = conv_ctx->not_improved_counter;
        alpha = ((VALUE_TYPE) samples_per_batch) * 2.0 / (no_samples + 1);
        alpha = (alpha > 1.0) ? 1.0 : alpha;
        ewa_wcssd = ewa_wcssd * (1 - alpha) + summed_batch_wcssd * alpha;
    }

    if (ewa_wcssd < ewa_wcssd_min) {
        not_improved_counter = 0;
        ewa_wcssd_min = ewa_wcssd;
    } else {
        not_improved_counter += 1;
    }

    if (not_improved_counter >= max_not_improved_counter) return 1;
    conv_ctx->ewa_wcssd = ewa_wcssd;
    conv_ctx->ewa_wcssd_min = ewa_wcssd_min;
    conv_ctx->not_improved_counter = not_improved_counter;
    return 0;
}

void post_process_iteration_minibatch(struct general_kmeans_context* ctx
                                      , uint32_t* chosen_sample_map
                                      , uint32_t max_not_improved_counter
                                      , struct convergence_context* conv_ctx) {
    uint64_t sample_id, samples_in_this_batch;
    ctx->wcssd = 0;
    samples_in_this_batch = 0;

    for (sample_id = 0; sample_id < ctx->samples->sample_count; sample_id++) {
        if (chosen_sample_map[sample_id]) {
            if (ctx->cluster_assignments[sample_id] != ctx->previous_cluster_assignments[sample_id]) ctx->no_changes += 1;
            ctx->wcssd += ctx->cluster_distances[sample_id];
            samples_in_this_batch += 1;
        }

    }

    ctx->total_no_calcs += ctx->done_calculations;
    if (ctx->track_time) ctx->duration_all_calcs = clock() - ctx->duration_all_calcs;

    ctx->converged = batch_convergence(ctx->samples->sample_count
                                        , samples_in_this_batch
                                        , ctx->wcssd
                                        , max_not_improved_counter
                                        , conv_ctx);
    ctx->wcssd = conv_ctx->ewa_wcssd;

    ctx->old_wcssd = ctx->wcssd;
}

void post_process_iteration(struct general_kmeans_context* ctx, struct kmeans_params *prms) {
    uint64_t sample_id;

    for (sample_id = 0; sample_id < ctx->samples->sample_count; sample_id++) {
        if (ctx->cluster_assignments[sample_id] != ctx->previous_cluster_assignments[sample_id]) ctx->no_changes += 1;
    }

    ctx->total_no_calcs += ctx->done_calculations;
    if (ctx->track_time) ctx->duration_all_calcs = (VALUE_TYPE) get_diff_in_microseconds(ctx->durations);

    /* calculate the objective. This is exact for kmeans/bv_kmeans */
    ctx->wcssd = sum_value_array(ctx->cluster_distances, ctx->samples->sample_count);

    if (fabs(ctx->wcssd - ctx->old_wcssd) < prms->tol || ctx->no_changes == 0) {
        ctx->converged = 1;
    }

    ctx->old_wcssd = ctx->wcssd;
}

void print_iteration_summary(struct general_kmeans_context* ctx, struct kmeans_params *prms, uint32_t iteration) {
    size_t hash_overhead;
    size_t clusters_nnz;
    size_t clusters_memory_consumption;
    VALUE_TYPE relative_dense_memory_consumption;
    int j;

    if (prms->verbose) LOG_INFO("Iteration %" PRINTF_INT32_MODIFIER "u wcssd %f change: %" PRINTF_INT64_MODIFIER "u clust: %" PRINTF_INT64_MODIFIER "u d:%" PRINTF_INT64_MODIFIER "u"
            , iteration
            , ctx->wcssd
            , ctx->no_changes
            , get_nnz_uint64_array(ctx->cluster_counts, ctx->no_clusters)
            , ctx->done_calculations);

    if (ctx->track_time && prms->verbose) LOG_INFO("Timings: all_calc=%" PRINTF_INT32_MODIFIER "d/up_clu=%" PRINTF_INT32_MODIFIER "d/overall_time=%.2f"
                          , (int32_t) (ctx->duration_all_calcs / 1000)
                          , (int32_t) (ctx->duration_update_clusters / 1000)
                          , get_diff_in_microseconds(ctx->tm_start));


    hash_overhead = 0;
    clusters_nnz = 0;
    for (j = 0; j < ctx->no_clusters; j++) {
        clusters_nnz += HASH_COUNT(ctx->clusters_raw[j]);
        hash_overhead += ((HASH_COUNT(ctx->clusters_raw[j]) * sizeof(struct keyvaluecount_hash))
                           +  HASH_OVERHEAD(hh, ctx->clusters_raw[j]));
    }

    clusters_memory_consumption = hash_overhead
                                  + (clusters_nnz * (sizeof(KEY_TYPE) + sizeof(VALUE_TYPE)));

    /* this determines how much memory the clusters use up compared to the case when
     * the clusters would have been stored dense.
     */
    relative_dense_memory_consumption = ((VALUE_TYPE) 100 * clusters_memory_consumption
                                        / (ctx->samples->dim * ctx->no_clusters * sizeof(VALUE_TYPE)));

    d_add_flist(&(prms->tr), "iteration_wcssd", ctx->wcssd);
    d_add_ilist(&(prms->tr), "iteration_changes", ctx->no_changes);
    d_add_ilist(&(prms->tr), "iteration_remaining_clusters"
                           , get_nnz_uint64_array(ctx->cluster_counts, ctx->no_clusters));
    d_add_ilist(&(prms->tr), "iteration_clusters_mem_consumption", clusters_memory_consumption);
    d_add_flist(&(prms->tr), "iteration_clusters_mem_consumption_relative_dense", relative_dense_memory_consumption);
    d_add_ilist(&(prms->tr), "iteration_clusters_nnz", clusters_nnz);
    d_add_ilist(&(prms->tr), "iteration_clusters_sparsity", (clusters_nnz * 100) / (ctx->samples->dim * ctx->no_clusters));
    d_add_ilist(&(prms->tr), "iteration_full_distance_calcs", ctx->done_calculations);
    d_add_flist(&(prms->tr), "iteration_durations_calcs", ((VALUE_TYPE) ctx->duration_all_calcs) / 1000.0);
    d_add_flist(&(prms->tr), "iteration_durations_update_clusters", ((VALUE_TYPE) ctx->duration_update_clusters) / 1000.0);
    d_add_flist(&(prms->tr), "iteration_durations", ((VALUE_TYPE) get_diff_in_microseconds(ctx->tm_start_iteration)));
    d_add_int(&(prms->tr), "no_iterations", iteration + 1);
}

struct kmeans_result* create_kmeans_result(struct kmeans_params *prms
                                          , struct general_kmeans_context* ctx) {
    struct kmeans_result* res;
    uint64_t i;
    d_add_float(&(prms->tr), "duration_kmeans", (VALUE_TYPE) get_diff_in_microseconds(ctx->tm_start));
    res = (struct kmeans_result*) calloc(1, sizeof(struct kmeans_result));
    /* create result cluster structure by with empty clusters */
    res->clusters = create_matrix_without_empty_elements(ctx->cluster_vectors,
                                                         ctx->no_clusters,
                                                         ctx->samples->dim,
                                                         NULL);

    res->initprms = ((struct initialization_params*) calloc(1, sizeof(struct initialization_params)));
    res->initprms->len_assignments = ctx->samples->sample_count;
    res->initprms->assignments = (uint64_t*) calloc(res->initprms->len_assignments, sizeof(uint64_t));

    res->initprms->len_initial_cluster_samples = ctx->no_clusters;
    res->initprms->initial_cluster_samples = (uint64_t*) calloc(res->initprms->len_initial_cluster_samples, sizeof(uint64_t));

    for(i = 0; i < ctx->no_clusters; i++) {
        res->initprms->initial_cluster_samples[i] = ctx->initial_cluster_samples[i];
    }

    if (prms->remove_empty) {
        struct assign_result assign_res;
        uint64_t no_filled;
        uint64_t* map;

        no_filled = 0;
        map = NULL;

        if (prms->verbose) LOG_INFO("Assigning all samples to clusters to find empty ones");

        /* assign all samples */
        assign_res = assign(ctx->samples, res->clusters, &(prms->stop));
        map = (uint64_t*) calloc(ctx->no_clusters, sizeof(uint64_t));

        for (i = 0; i < ctx->no_clusters; i++) {
            if (assign_res.counts[i] != 0) {
                map[i] = no_filled;
                no_filled += 1;
            }
        }

        if (!(prms->stop)) {
            free_csr_matrix(res->clusters);
            free_null(res->clusters);
            res->clusters = create_matrix_without_empty_elements(ctx->cluster_vectors,
                                                                 ctx->no_clusters,
                                                                 ctx->samples->dim,
                                                                 assign_res.counts);

            for (i = 0; i < res->initprms->len_assignments; i++) {
                res->initprms->assignments[i] = map[ctx->cluster_assignments[i]];
            }

            if (prms->verbose) LOG_INFO("Remaining clusters after deleting empty ones = %lu"
                                      , res->clusters->sample_count);
        }
        d_add_float(&(prms->tr), "wcssd_kmeans_with_remove_empty"
                    , sum_value_array(assign_res.distances, assign_res.len_assignments));

        free_assign_result(&assign_res);
        free_null(map);
        d_add_float(&(prms->tr), "duration_kmeans_with_remove_empty"
                    , (VALUE_TYPE) get_diff_in_microseconds(ctx->tm_start));
    } else {
        for (i = 0; i < res->initprms->len_assignments; i++) {
            res->initprms->assignments[i] = ctx->cluster_assignments[i];
        }
    }

    d_add_int(&(prms->tr), "no_clusters_remaining", res->clusters->sample_count);
    return res;
}

void initialize_general_context(struct kmeans_params *prms
                                , struct general_kmeans_context* ctx
                                , struct csr_matrix* samples) {

    uint64_t i;
    VALUE_TYPE old_wcssd_;
    memset(ctx, 0, sizeof(struct general_kmeans_context));

    if (prms->verbose) LOG_INFO("----------------");
    if (prms->verbose) LOG_INFO("%s", KMEANS_ALGORITHM_NAMES[prms->kmeans_algorithm_id]);
    if (prms->verbose) LOG_INFO("----------------");

    if (KMEANS_PREINIT_FUNCTIONS[prms->init_id]) {
        KMEANS_PREINIT_FUNCTIONS[prms->init_id](samples, prms);
    }

    d_add_subint(&(prms->tr), "general_params", "no_clusters", prms->no_clusters);
    d_add_substring(&(prms->tr), "general_params", "algorithm", (char*) KMEANS_ALGORITHM_NAMES[prms->kmeans_algorithm_id]);
    d_add_subint(&(prms->tr), "general_params", "seed", prms->seed);
    d_add_subint(&(prms->tr), "general_params", "remove_empty", prms->remove_empty);
    d_add_subint(&(prms->tr), "general_params", "iteration_limit", prms->iteration_limit);
    d_add_subfloat(&(prms->tr), "general_params", "tol", prms->tol);
    d_add_substring(&(prms->tr), "general_params", "init", (char*) KMEANS_INIT_NAMES[prms->init_id]);
    d_add_subint(&(prms->tr), "general_params", "no_cores_used", omp_get_max_threads());

    ctx->samples = samples;

    gettimeofday(&(ctx->tm_start), NULL);

    /* enables time tracking of specific parts of the source code */
    ctx->track_time = 1;

    /* calculate ||s|| for every s in samples */
    calculate_matrix_vector_lengths(ctx->samples, &ctx->vector_lengths_samples);

    /* create clusters data structures */
    if (prms->no_clusters > ctx->samples->sample_count) {
        prms->no_clusters = ctx->samples->sample_count;
    }

    ctx->clusters_raw = (struct keyvaluecount_hash**) calloc(prms->no_clusters, sizeof(struct keyvaluecount_hash*));
    ctx->cluster_vectors = (struct sparse_vector*) calloc(prms->no_clusters, sizeof(struct sparse_vector));
    ctx->no_clusters = prms->no_clusters;

    ctx->cluster_counts = (uint64_t*) calloc(prms->no_clusters, sizeof(uint64_t));
    ctx->cluster_assignments = (uint64_t*) calloc(ctx->samples->sample_count, sizeof(uint64_t));
    ctx->initial_cluster_samples = (uint64_t*) calloc(prms->no_clusters, sizeof(uint64_t));

    ctx->cluster_distances = (VALUE_TYPE*) calloc(ctx->samples->sample_count, sizeof(VALUE_TYPE));

    ctx->was_assigned = (uint32_t*) calloc(ctx->samples->sample_count, sizeof(uint32_t));

    ctx->previous_cluster_assignments = NULL;

    gettimeofday(&(ctx->durations), NULL);

    /* do initialization */
    KMEANS_INIT_FUNCTIONS[prms->init_id](ctx, prms);

    d_add_float(&(prms->tr), "duration_init", (VALUE_TYPE) get_diff_in_microseconds(ctx->durations));

    /* calculate the distance from the samples to their initial clusters */
    calculate_initial_distances_clusters(ctx->samples
                                               , ctx->cluster_vectors
                                               , ctx->no_clusters
                                               , ctx->cluster_assignments
                                               , ctx->vector_lengths_samples
                                               , ctx->cluster_distances);

    /* every cluster is assumed to not have changed in the beginning */
    ctx->clusters_not_changed = (uint32_t*) calloc(prms->no_clusters, sizeof(uint32_t));
    for (i = 0; i < prms->no_clusters; i++) {
        ctx->clusters_not_changed[i] = 0;
    }

    /* calculate the initial wcssd after initialization */
    old_wcssd_ = 0;
    #pragma omp parallel for reduction(+:old_wcssd_)
    for (i = 0; i < ctx->samples->sample_count; i++) {
        old_wcssd_ += ctx->cluster_distances[i];
    }

    ctx->old_wcssd = old_wcssd_;

    calculate_vector_list_lengths(ctx->cluster_vectors, ctx->no_clusters, &(ctx->vector_lengths_clusters));

    if (prms->verbose) LOG_INFO("old_wcssd %f, input_samples = %" PRINTF_INT64_MODIFIER "u, input_dimension = %" PRINTF_INT64_MODIFIER "u, input_average_nnz = %" PRINTF_INT64_MODIFIER "u, overall_time_before_first_iteration %.2f"
            , ctx->old_wcssd
            , ctx->samples->sample_count
            , ctx->samples->dim, ctx->samples->pointers[ctx->samples->sample_count] / ctx->samples->sample_count
            , get_diff_in_microseconds(ctx->tm_start));   /* this output is needed to verify that all algorithms start at the same starting position */

    /* indicates if kmeans has converged */
    ctx->converged = 0;

    d_add_float(&(prms->tr), "initial_wcssd", ctx->old_wcssd);
    d_add_int(&(prms->tr), "input_samples", ctx->samples->sample_count);
    d_add_int(&(prms->tr), "input_dimension", ctx->samples->dim);
    d_add_int(&(prms->tr), "input_annz", ctx->samples->pointers[ctx->samples->sample_count] / ctx->samples->sample_count);

}

void search_samples_block_vectors(struct kmeans_params *prms
                                   , struct csr_matrix* samples
                                   , VALUE_TYPE desired_annz
                                   , struct csr_matrix* block_vectors_samples
                                   , uint64_t *block_vectors_dim) {

    determine_block_vectors_for_matrix(samples
                                       , desired_annz
                                       , block_vectors_samples
                                       , block_vectors_dim);

    d_add_subint(&(prms->tr), "block_vector_data", "dim", block_vectors_samples->dim);
    d_add_subint(&(prms->tr), "block_vector_data", "annz", (uint64_t) (block_vectors_samples->pointers[block_vectors_samples->sample_count] / block_vectors_samples->sample_count));
    d_add_subint(&(prms->tr), "block_vector_data", "annz_samples", (uint64_t) (samples->pointers[samples->sample_count] / samples->sample_count));


    if (prms->verbose) LOG_INFO("n_blockvector_mtrx = %" PRINTF_INT64_MODIFIER "u, average_nnz_blockvektor_mtrx = %" PRINTF_INT64_MODIFIER "u"
            , block_vectors_samples->dim
            , block_vectors_samples->pointers[block_vectors_samples->sample_count] / block_vectors_samples->sample_count);
}

void switch_to_shifted_clusters(struct general_kmeans_context* ctx) {
    /* free old clusters as they are replaced with the shifted ones */
    uint64_t i;

    for (i = 0; i  < ctx->no_clusters; i++) {
        if (ctx->cluster_vectors[i].keys != ctx->shifted_cluster_vectors[i].keys) {
            free_null(ctx->cluster_vectors[i].keys);
            free_null(ctx->cluster_vectors[i].values);
            ctx->cluster_vectors[i].nnz = ctx->shifted_cluster_vectors[i].nnz;
            ctx->cluster_vectors[i].keys = ctx->shifted_cluster_vectors[i].keys;
            ctx->cluster_vectors[i].values = ctx->shifted_cluster_vectors[i].values;
        }
    }
    free_null(ctx->shifted_cluster_vectors);


    free_null(ctx->vector_lengths_clusters);
    ctx->vector_lengths_clusters = ctx->vector_lengths_shifted_clusters;
}

void calculate_shifted_clusters_general(struct general_kmeans_context* ctx
                                        , uint32_t* active_sample_map
                                        , uint32_t update_type) {
    uint64_t* was_cluster_hashmap_changed;
    uint64_t j;

    if (ctx->track_time) gettimeofday(&(ctx->durations), NULL);

    was_cluster_hashmap_changed = (uint64_t*) calloc(ctx->no_clusters, sizeof(uint64_t));

    #pragma omp parallel for schedule(dynamic, 1000)
    for (j = 0; j < ctx->no_clusters; j++) {
        ctx->clusters_not_changed[j] = 1;
    }

    /* update cluster_centers if needed */
    for (j = 0; j < ctx->samples->sample_count; j++) {
        if (update_type == UPDATE_TYPE_KMEANS) {
            if ((ctx->previous_cluster_assignments[j]
                != ctx->cluster_assignments[j]) || !ctx->was_assigned[j]) {
                    KEY_TYPE* keys;
                    VALUE_TYPE* values;
                    uint64_t nnz;
                    keys = ctx->samples->keys + ctx->samples->pointers[j];
                    values = ctx->samples->values + ctx->samples->pointers[j];
                    nnz = ctx->samples->pointers[j + 1] - ctx->samples->pointers[j];

                    if (ctx->was_assigned[j]) {
                        remove_sample_from_hashmap(ctx->clusters_raw, keys, values, nnz, ctx->previous_cluster_assignments[j]);
                        ctx->cluster_counts[ctx->previous_cluster_assignments[j]] -= 1;
                        ctx->clusters_not_changed[ctx->previous_cluster_assignments[j]] = 0;
                    }

                    was_cluster_hashmap_changed[ctx->cluster_assignments[j]] += add_sample_to_hashmap(ctx->clusters_raw, keys, values, nnz, ctx->cluster_assignments[j]);
                    ctx->cluster_counts[ctx->cluster_assignments[j]] += 1;
                    ctx->clusters_not_changed[ctx->cluster_assignments[j]] = 0;
                    ctx->was_assigned[j] = 1;
            }
        } else if (update_type == UPDATE_TYPE_MINIBATCH_KMEANS) {
            if (active_sample_map[j]) {
                    KEY_TYPE* keys;
                    VALUE_TYPE* values;
                    uint64_t nnz;
                    keys = ctx->samples->keys + ctx->samples->pointers[j];
                    values = ctx->samples->values + ctx->samples->pointers[j];
                    nnz = ctx->samples->pointers[j + 1] - ctx->samples->pointers[j];
                    was_cluster_hashmap_changed[ctx->cluster_assignments[j]]
                       += add_sample_to_hashmap_minibatch_kmeans(ctx->clusters_raw
                                                                 , keys
                                                                 , values
                                                                 , nnz
                                                                 , ctx->cluster_assignments[j]
                                                                 , ctx->cluster_counts[ctx->cluster_assignments[j]]);
                    ctx->cluster_counts[ctx->cluster_assignments[j]] += 1;
                    ctx->clusters_not_changed[ctx->cluster_assignments[j]] = 0;
                    ctx->was_assigned[j] = 1;
            }
        }
    }

    ctx->shifted_cluster_vectors = (struct sparse_vector*) calloc(ctx->no_clusters, sizeof(struct csr_matrix));

    for (j = 0; j < ctx->no_clusters; j++) {

        if (was_cluster_hashmap_changed[j]) {
            HASH_SORT((ctx->clusters_raw)[j], id_sort);
        }

        if (ctx->clusters_not_changed[j]) {
            /* cluster was not changed! use old cluster as shifted */
            ctx->shifted_cluster_vectors[j].nnz = ctx->cluster_vectors[j].nnz;
            ctx->shifted_cluster_vectors[j].keys = ctx->cluster_vectors[j].keys;
            ctx->shifted_cluster_vectors[j].values = ctx->cluster_vectors[j].values;
        } else {
            /* cluster has changed! adapt it */

            uint64_t local_feature_count;
            struct keyvaluecount_hash *current_item, *tmp;
            current_item = NULL;
            tmp = NULL;

            ctx->shifted_cluster_vectors[j].nnz = HASH_COUNT(ctx->clusters_raw[j]);
            /* printf("adapt cluster %u %u", j, ctx->shifted_cluster_vectors[j].nnz); */
            if (ctx->shifted_cluster_vectors[j].nnz > 0) {
                ctx->shifted_cluster_vectors[j].keys = (KEY_TYPE*) calloc(ctx->shifted_cluster_vectors[j].nnz, sizeof(KEY_TYPE));
                ctx->shifted_cluster_vectors[j].values = (VALUE_TYPE*) calloc(ctx->shifted_cluster_vectors[j].nnz, sizeof(VALUE_TYPE));
            }

            local_feature_count = 0;
            HASH_ITER(hh, ctx->clusters_raw[j], current_item, tmp) {
              ctx->shifted_cluster_vectors[j].keys[local_feature_count] = current_item->id;
              if (update_type == UPDATE_TYPE_MINIBATCH_KMEANS) {
                  ctx->shifted_cluster_vectors[j].values[local_feature_count] = current_item->val;
              } else {
                  ctx->shifted_cluster_vectors[j].values[local_feature_count] = current_item->val / ctx->cluster_counts[j];
              }
              local_feature_count += 1;
            }

        }
    }

    /* only recalculate for clusters which have actually changed */
    ctx->vector_lengths_shifted_clusters = (VALUE_TYPE*) calloc(ctx->no_clusters, sizeof(VALUE_TYPE));
    memcpy(ctx->vector_lengths_shifted_clusters, ctx->vector_lengths_clusters, ctx->no_clusters * sizeof(VALUE_TYPE));

    update_vector_list_lengths(ctx->shifted_cluster_vectors
                               , ctx->no_clusters
                               , ctx->clusters_not_changed
                               , ctx->vector_lengths_shifted_clusters);

    free_null(was_cluster_hashmap_changed);

    if (ctx->track_time) ctx->duration_update_clusters = (VALUE_TYPE) get_diff_in_microseconds(ctx->durations);
}

void calculate_shifted_clusters(struct general_kmeans_context* ctx) {
    calculate_shifted_clusters_general(ctx, NULL, UPDATE_TYPE_KMEANS);
}

void calculate_shifted_clusters_minibatch_kmeans(struct general_kmeans_context* ctx
                                                , uint32_t* active_sample_map) {
    calculate_shifted_clusters_general(ctx
                                      , active_sample_map
                                      , UPDATE_TYPE_MINIBATCH_KMEANS);
}

void create_kmeans_cluster_groups(struct sparse_vector *clusters_list
                                  , uint64_t no_clusters
                                  , uint64_t dim
                                  , struct group** groups
                                  , uint64_t* no_groups) {
    uint64_t i;
    struct kmeans_result* res;
    uint64_t* cluster_counters;
    uint32_t stop;
    struct csr_matrix clusters;
    struct assign_result assign_res;

    struct kmeans_params prms;
    prms.kmeans_algorithm_id = ALGORITHM_KMEANS;
    prms.no_clusters = *no_groups;
    prms.seed = 1;
    prms.iteration_limit = 5;
    prms.verbose = 0;
    prms.init_id = KMEANS_INIT_KMPP;
    prms.tol = 1e-6;
    prms.remove_empty = 0;
    prms.stop = 0;
    prms.tr = NULL;
    stop = 0;

    sparse_vector_list_to_csr_matrix(clusters_list
                                          , no_clusters
                                          , dim
                                          , &clusters);

    /* cluster the cluster centers into no_group groups */
    res = bv_kmeans(&clusters, &prms);
    *no_groups = res->clusters->sample_count;

    /* assign clusters to groups */
    assign_res = assign(&clusters, res->clusters, &stop);

    *groups = (struct group*) calloc(*no_groups, sizeof(struct group));
    cluster_counters = (uint64_t*) calloc(*no_groups, sizeof(uint64_t));

    for (i = 0; i < *no_groups; i++) {
        (*groups)[i].no_clusters = assign_res.counts[i];
        (*groups)[i].clusters = (uint64_t*) calloc((*groups)[i].no_clusters
                                                   , sizeof(uint64_t));
    }

    for (i = 0; i < clusters.sample_count; i++) {
        uint64_t group;
        group = assign_res.assignments[i];
        (*groups)[group].clusters[cluster_counters[group]] = i;
        cluster_counters[group]++;
    }
    free_cdict(&(prms.tr));
    free(cluster_counters);
    free_kmeans_result(res);
    free_assign_result(&assign_res);
    free_csr_matrix(&clusters);
}

void calculate_initial_distances_clusters(struct csr_matrix *samples
                                           , struct sparse_vector* clusters
                                           , uint64_t no_clusters
                                           , uint64_t* cluster_assignments
                                           , VALUE_TYPE* vector_lengths_samples
                                           , VALUE_TYPE* cluster_distances) {
    VALUE_TYPE* vector_lengths_clusters;
    uint64_t sample_id;

    calculate_vector_list_lengths(clusters, no_clusters, &vector_lengths_clusters);

    #pragma omp parallel for schedule(dynamic, 1000)
    for (sample_id = 0; sample_id < samples->sample_count; sample_id++) {
        cluster_distances[sample_id]
                 = euclid_vector_list(samples, sample_id
                          , clusters, cluster_assignments[sample_id]
                          , vector_lengths_samples
                          , vector_lengths_clusters);
    }

    free(vector_lengths_clusters);
}

void calculate_distance_clustersold_to_clustersnew(VALUE_TYPE* distance_clustersold_to_clustersnew
                                                   , struct sparse_vector* new_clusters
                                                   , struct sparse_vector* old_clusters
                                                   , uint64_t no_clusters
                                                   , VALUE_TYPE* vector_length_new_clusters
                                                   , VALUE_TYPE* vector_length_old_clusters
                                                   , uint32_t* clusters_not_changed) {
    uint64_t cluster_id;

    #pragma omp parallel for schedule(dynamic, 1000)
    for (cluster_id = 0; cluster_id < no_clusters; cluster_id++) {

        if (clusters_not_changed[cluster_id] == 0) {
            distance_clustersold_to_clustersnew[cluster_id]
               = euclid_vector(new_clusters[cluster_id].keys
                               , new_clusters[cluster_id].values
                               , new_clusters[cluster_id].nnz
                               , old_clusters[cluster_id].keys
                               , old_clusters[cluster_id].values
                               , old_clusters[cluster_id].nnz
                               , vector_length_new_clusters[cluster_id]
                               , vector_length_old_clusters[cluster_id]);
        } else {
            distance_clustersold_to_clustersnew[cluster_id] = 0;
        }
    }
}
