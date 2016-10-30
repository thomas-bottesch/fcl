#include "fcl_kmeans_commons.h"
#include "mx_fcl_kmeans_fit.h"
#include <stdlib.h>
#include "../../utils/fcl_logging.h"

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, 
    const mxArray *prhs[]) {

    struct csr_matrix *clusters;        // will hold cluster centers
    struct csr_matrix *input_dataset;
    uint64_t num_clusters_out;      // the final number of clusters (could be less than k)
    uint64_t nnz_out;               // the final number of nonzeros;
    struct kmeans_params *prms;          // internal parameters passed to fcl library
    double * X_out;                     // the data in matlab format (len: nnz_out)
    mwIndex * irs_out;                  // specifies the row indices (len: nnz_out)
    mwIndex * jcs_out;                  // specifies how many values in each column (len: num_clusters_out)
    mwSize max_mwsize_value;

    max_mwsize_value = (mwSize) -1;

    uint32_t fitted_successfully;

    prms = NULL;
    clusters = NULL;
    input_dataset = NULL;

    // Check if right number of parameters
    if (!(nlhs == 1 || nlhs == 2) || nrhs <2 || nrhs >3) {
        LOG_INFO("Wrong input. Usage: C = fcl_kmeans(X, k, opts).");
        return;
    }

    if (nrhs == 3 && !mxIsStruct(prhs[2])) {
        LOG_INFO("Wrong input. Third (optional) argument should be struct with additional options.");
        return;
    }

    fitted_successfully = fcl_kmeans_fit(prhs[0]
                                         , prhs[1]
                                         , nrhs == 3 ? prhs[2] : NULL
                                         , &prms
                                         , &clusters
                                         , &input_dataset);

    if (!fitted_successfully) goto end;

    if (prms->stop) {
        goto end;
    }

    if (prms->verbose) {
        LOG_INFO("Algorithm finished.");
    }

    // output
    num_clusters_out = clusters->sample_count;
    nnz_out = clusters->pointers[num_clusters_out];

    /* check if the resulting sizes fit with the output size */
    if (num_clusters_out <= max_mwsize_value
        && nnz_out <= max_mwsize_value
        && clusters->dim <= max_mwsize_value) {

        // allocate space for output
        plhs[0] = mxCreateSparse(mxGetM(prhs[0]), (mwSize) num_clusters_out, (mwSize) nnz_out, mxREAL);
        irs_out = mxGetIr(plhs[0]);
        jcs_out = mxGetJc(plhs[0]);
        X_out = mxGetPr(plhs[0]);

        // convert back to from matlab csc format
        convert_to_matlab_csc_matrix(&clusters, X_out, irs_out, jcs_out, num_clusters_out, nnz_out);
    } else {
        LOG_ERROR("Error. Resulting cluster matrix has a size which is too large to be handled by your matlab installation!\n");
        return;
    }

    if (nlhs == 2) {
        plhs[1] = create_struct(&(prms->tr));
    }

end:
    // clean up
    if (prms) {
        free_cdict(&(prms->tr));
        free_null(prms);
    }

    if (clusters) {
        free_csr_matrix(clusters);
        free_null(clusters);
    }

    if (input_dataset) {
        free_csr_matrix(input_dataset);
        free_null(input_dataset);
    }
}
