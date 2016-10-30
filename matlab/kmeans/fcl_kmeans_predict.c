#include "fcl_kmeans_commons.h"
#include "../../utils/matrix/csr_matrix/csr_assign.h"
#include <stdlib.h>
#include "../../utils/fcl_logging.h"

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, 
    const mxArray *prhs[]) {

    struct csr_matrix *clusters;        // will hold cluster centers
    struct csr_matrix *input_dataset;
    struct assign_result assign_res;
    struct kmeans_params prms;          // internal parameters passed to fcl library
    uint32_t stop;

    memset(&assign_res, 0, sizeof(struct assign_result));
    input_dataset = NULL;
    clusters = NULL;
    prms.tr = NULL;

    // Check if right number of parameters
    if (!(nlhs >= 1 && nlhs <= 2) || nrhs < 2 || nrhs > 3) {
        LOG_INFO("Wrong input. Usage: [IDX] = fcl_kmeans_predict(C, X, opts).");
        LOG_INFO("Or: [IDX, SUMD] = fcl_kmeans_predict(C, X, opts).");
        goto end;
    }

    if (!load_dataset(prhs[0], &clusters)) {
        goto end;
    }

    if (!load_dataset(prhs[1], &input_dataset)) {
        goto end;
    }

    // read optional input parameters if available
    if (nrhs < 3) {
        LOG_INFO("Using default parameters.");
    } else {
        int successful = read_optional_params(&prms, prhs[2]);
        if (!successful) goto end;
    }

    /* set stop to zero */
    stop = 0;

    /* assign */
    assign_res = assign(input_dataset, clusters, &stop);

    if (stop) {
        goto end;
    }

    if (prms.verbose) {
        LOG_INFO("Kmeans predict (assign) finished.");
    }

    plhs[0] = convert_uint64_array_to_mxarray(assign_res.assignments
                                               , assign_res.len_assignments);

    if (nlhs >= 2) {
        VALUE_TYPE* arr;
        uint64_t i;
        plhs[1] = mxCreateNumericMatrix(clusters->sample_count, 1, mxDOUBLE_CLASS, mxREAL);
        arr = (VALUE_TYPE*) mxGetData(plhs[1]);
        for (i = 0; i < assign_res.len_assignments; i++) {
            arr[assign_res.assignments[i]] += assign_res.distances[i];
        }
    }

end:
    free_cdict(&(prms.tr));

    if (input_dataset) {
        free_csr_matrix(input_dataset);
        free_null(input_dataset);
    }

    if (clusters) {
        free_csr_matrix(clusters);
        free_null(clusters);
    }

    free_assign_result(&assign_res);
}
