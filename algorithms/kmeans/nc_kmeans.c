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
struct kmeans_result* nc_kmeans(struct csr_matrix* samples, struct kmeans_params *prms) {

    uint32_t i;
    uint64_t j;
    struct kmeans_result* res;
    uint32_t disable_optimizations;

    /* nc_kmeans: contains all samples which are eligible for the cluster
     * no change optimization.
     */
    uint32_t *eligible_for_cluster_no_change_optimization;
    struct general_kmeans_context ctx;

    initialize_general_context(prms, &ctx, samples);

    eligible_for_cluster_no_change_optimization = (uint32_t*) calloc(ctx.samples->sample_count, sizeof(uint32_t));

    for (i = 0; i < prms->iteration_limit && !ctx.converged && !prms->stop; i++) {
        /* track how many blockvector calculations were made / saved */
        uint64_t saved_calculations_prev_cluster;

        /* reset all calculation counters */
        saved_calculations_prev_cluster = 0;

        /* initialize data needed for the iteration */
        pre_process_iteration(&ctx);

        #pragma omp parallel for schedule(dynamic, 1000)
        for (j = 0; j < ctx.samples->sample_count; j++) {
            /* iterate over all samples */

            VALUE_TYPE dist;
            uint64_t cluster_id, sample_id;

            if (omp_get_thread_num() == 0) check_signals(&(prms->stop));

            if (!prms->stop) {
                sample_id = j;

                for (cluster_id = 0; cluster_id < ctx.no_clusters; cluster_id++) {
                    /* iterate over all cluster centers */

                    /* if we are not in the first iteration and this cluster is empty, continue to next cluster */
                    if (i != 0 && ctx.cluster_counts[cluster_id] == 0) continue;

                    /* we already know the distance to the cluster from last iteration */
                    if (cluster_id == ctx.previous_cluster_assignments[sample_id]) continue;

                    /* clusters which did not move in the last iteration can be skipped if the sample is eligible */
                    if (eligible_for_cluster_no_change_optimization[sample_id] && ctx.clusters_not_changed[cluster_id]) {
                        /* cluster did not move and sample was eligible for this check. distance to this cluster can not be less than to our best from last iteration */
                        saved_calculations_prev_cluster += 1;
                        goto end;
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

        post_process_iteration(&ctx, prms);

        /* shift clusters to new position */
        calculate_shifted_clusters(&ctx);
        switch_to_shifted_clusters(&ctx);

        d_add_ilist(&(prms->tr), "iteration_nc_calcs_saved", saved_calculations_prev_cluster);

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

        print_iteration_summary(&ctx, prms, i);

        /* print block vector statistics */
        if (prms->verbose) LOG_INFO("Saved calculations previous cluster pc:%" PRINTF_INT64_MODIFIER "u", saved_calculations_prev_cluster);
    }

    if (prms->verbose) LOG_INFO("total total_no_calcs = %" PRINTF_INT64_MODIFIER "u", ctx.total_no_calcs);

    res = create_kmeans_result(prms, &ctx);

    free_general_context(&ctx, prms);
    free_null(eligible_for_cluster_no_change_optimization);


    return res;
}
