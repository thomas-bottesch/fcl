#include "kmeanspp.h"
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
struct kmeans_result* bv_kmeanspp(struct csr_matrix* samples, struct kmeans_params *prms) {
    struct general_kmeans_context ctx;
    VALUE_TYPE desired_kmppbv_annz;
    VALUE_TYPE desired_bv_annz;
    struct kmeans_result* res;
    prms->init_id = KMEANS_INIT_KMPP;

    if (prms->kmeans_algorithm_id == ALGORITHM_KMEANSPP) {
        d_add_subfloat(&(prms->tr), "additional_params", "kmpp_bv_annz", 0.0);
    } else if (prms->kmeans_algorithm_id == ALGORITHM_BV_KMEANSPP) {
        d_get_subfloat_default(&(prms->tr),
                               "additional_params",
                               "kmpp_bv_annz",
                               d_get_subfloat_default(&(prms->tr),
                                                      "additional_params",
                                                      "bv_annz",
                                                      0.3));
    } else if (prms->kmeans_algorithm_id == ALGORITHM_PCA_KMEANSPP) {
        if (prms->ext_vects == NULL) {
            if (prms->verbose) LOG_INFO("pca_kmeans++ was requested but no external vectors were supplied. Ignoring the pca optimizations!");
        }


        d_add_subint(&(prms->tr), "additional_params", "kmpp_use_pca", prms->ext_vects != NULL);
    }

    /* kmeans++ is done inside the initialize_general_context */
    initialize_general_context(prms, &ctx, samples);


    res = create_kmeans_result(prms, &ctx);
    free_general_context(&ctx, prms);
    return res;
}
