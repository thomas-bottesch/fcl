#include "fcl_kmeans_commons.h"
#include "mx_fcl_kmeans_fit.h"
#include "../../utils/matrix/csr_matrix/csr_assign.h"
#include <stdlib.h>
#include "../../utils/fcl_logging.h"
#include "../../algorithms/kmeans/kmeans_utils.h"

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, 
    const mxArray *prhs[]) {

    struct kmeans_result *res;        // will hold cluster centers
    struct csr_matrix *input_dataset;
    uint64_t num_clusters_out;      // the final number of clusters (could be less than k)
    uint64_t nnz_out;               // the final number of nonzeros;
    struct kmeans_params *prms;          // internal parameters passed to fcl library
    double * X_out;                     // the data in matlab format (len: nnz_out)
    mwIndex * irs_out;                  // specifies the row indices (len: nnz_out)
    mwIndex * jcs_out;                  // specifies how many values in each column (len: num_clusters_out)
    struct assign_result assign_res;
    mwSize max_mwsize_value;
    int fitted_successfully;

    max_mwsize_value = (mwSize) -1;

    memset(&assign_res, 0, sizeof(struct assign_result));
    prms = NULL;
    res = NULL;
    input_dataset = NULL;

    // Check if right number of parameters
    if (!(nlhs >= 1 && nlhs <= 5) || nrhs <2 || nrhs >3) {
        LOG_INFO("Wrong input. Usage: [IDX] = fcl_kmeans(X, k, opts).");
        LOG_INFO("Or: [IDX, C] = fcl_kmeans(X, k, opts).");
        LOG_INFO("Or: [IDX, C, SUMD] = fcl_kmeans(X, k, opts).");
        LOG_INFO("Or: [IDX, C, SUMD, T] = fcl_kmeans(X, k, opts).");
        LOG_INFO("Or: [IDX, C, SUMD, T, INITPRMS] = fcl_kmeans(X, k, opts).");
        goto end;
    }

    if (nrhs == 3 && !mxIsStruct(prhs[2])) {
        LOG_ERROR("Wrong input. Third (optional) argument should be struct with additional options.");
        goto end;
    }

    fitted_successfully = fcl_kmeans_fit(prhs[0]
                                         , prhs[1]
                                         , nrhs == 3 ? prhs[2] : NULL
                                         , &prms
                                         , &res
                                         , &input_dataset);
    if (!fitted_successfully || prms->stop) goto end;
    if (prms->verbose) {
        LOG_INFO("Kmeans fit finished.");
    }

    /* assign */
    assign_res = assign(input_dataset, res->clusters, &(prms->stop));

    if (prms->verbose) {
        LOG_INFO("Kmeans predict (assign) finished.");
    }

    plhs[0] = convert_uint64_array_to_mxarray(assign_res.assignments,
                                              assign_res.len_assignments);

    // output
    if (nlhs >= 2) {
        num_clusters_out = res->clusters->sample_count;
        nnz_out = res->clusters->pointers[num_clusters_out];

        /* check if the resulting sizes fit with the output size */
        if (num_clusters_out <= max_mwsize_value
            && nnz_out <= max_mwsize_value) {

            // allocate space for output
            plhs[1] = mxCreateSparse(input_dataset->dim, (mwSize) num_clusters_out, (mwSize) nnz_out, mxREAL);
            irs_out = mxGetIr(plhs[1]);
            jcs_out = mxGetJc(plhs[1]);
            X_out = mxGetPr(plhs[1]);

            // convert back to from matlab csc format
            convert_to_matlab_csc_matrix(&res->clusters, X_out,irs_out,jcs_out,num_clusters_out,nnz_out);
        } else {
            LOG_ERROR("Error. Resulting cluster matrix has a size which is too large to be handled by your matlab installation!");
            goto end;
        }
    }

    if (nlhs >= 3) {
        VALUE_TYPE* arr;
        uint64_t i;
        plhs[2] = mxCreateNumericMatrix(res->clusters->sample_count, 1, mxDOUBLE_CLASS, mxREAL);
        arr = (VALUE_TYPE*) mxGetData(plhs[2]);
        for (i = 0; i < assign_res.len_assignments; i++) {
            arr[assign_res.assignments[i]] += assign_res.distances[i];
        }
    }

    if (nlhs >= 4) {
        plhs[3] = create_struct(&(prms->tr));
    }

    if (nlhs >= 5) {
        plhs[4] = create_init_params_struct(res->initprms);
    }

end:
    // clean up
    if (prms) {
        free_cdict(&(prms->tr));
        free_null(prms);
    }

    if (res) {
        free_kmeans_result(res);
        res = NULL;
    }

    if (input_dataset) {
        free_csr_matrix(input_dataset);
        free_null(input_dataset);
    }

    free_assign_result(&assign_res);
}
