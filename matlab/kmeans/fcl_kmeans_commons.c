#include "fcl_kmeans_commons.h"
#include "../../utils/matrix/csr_matrix/csr_load_matrix.h"
#include "../../utils/fcl_logging.h"
#include <stdlib.h>

uint32_t myIsScalar(const mxArray *dat) {
    int32_t number_of_dims;
    const mwSize *dim_array;

   number_of_dims = mxGetNumberOfDimensions(dat);
   if (number_of_dims != 2) return 0;

   dim_array = mxGetDimensions(dat);
   if (dim_array[0] != 1 && dim_array[1] != 1) return 0;
   return 1;
}

uint32_t isNumericScalar(const mxArray *dat) {
    if (dat != NULL && myIsScalar(dat)) {
        if (mxIsNumeric(dat)) return 1;
    }
    return 0;
}

uint32_t isCharScalar(const mxArray *dat) {
    if (dat != NULL && myIsScalar(dat)) {
        if (mxIsChar(dat)) return 1;
    }
    return 0;
}

// since matlab stores sparse matrices in csc format we require that points are stored columnwise;
// then the conversion to a csr format with points as rows becomes easy
void convert_to_csr_matrix(struct csr_matrix **mtrx, double *X, mwIndex * irs, mwIndex * jcs, mwSize nnz, mwSize num, mwSize dim) {
    uint64_t i;

    *mtrx = (struct csr_matrix*) malloc(sizeof(struct csr_matrix));
    (*mtrx)->values = NULL;
    (*mtrx)->keys = NULL;
    (*mtrx)->pointers = NULL;
    (*mtrx)->sample_count = 0;
    (*mtrx)->dim = 0;

    (*mtrx)->values = (VALUE_TYPE *) malloc(nnz * sizeof(VALUE_TYPE));
    (*mtrx)->keys = (KEY_TYPE *) malloc(nnz * sizeof(KEY_TYPE));
    (*mtrx)->sample_count = (uint64_t) num;
    (*mtrx)->dim = (uint64_t) dim;
    (*mtrx)->pointers = (POINTER_TYPE *) malloc((num + 1) * sizeof(POINTER_TYPE));
    (*mtrx)->pointers[0] = 0;

    for (i = 0; i < nnz; i++) {
        (*mtrx)->keys[i] = (KEY_TYPE) irs[i];
        (*mtrx)->values[i] = (VALUE_TYPE) X[i];
    }

    for (i = 0; i < num + 1; i++) {
        (*mtrx)->pointers[i] = (POINTER_TYPE) jcs[i];
    }
}

void check_signals(uint32_t* stop) {
    #ifdef MATLAB_MEX_FILE
    if (!(*stop)) {
        if (utIsInterruptPending()) {        /* check for a Ctrl-C event */
            mexPrintf("Ctrl-C Detected. END\n\n");
            mexEvalString("pause(.001);");
            *stop = 1;
        }
    }
    #endif
}

void convert_to_matlab_csc_matrix(struct csr_matrix **clusters, double * X, mwIndex * irs, mwIndex * jcs, uint64_t num, uint64_t nnz ) {
    uint64_t i;

    for (i = 0; i< nnz; i++) {
        irs[i] = (*clusters)->keys[i];
        X[i] = (*clusters)->values[i];
    }

    for (i = 0; i< num+1; i++) {
        jcs[i] = (*clusters)->pointers[i];
    }
}

int32_t convert_struct_field_to_uint32(const mxArray* entryStruct, char* fieldname, uint32_t* k) {
    const mxArray  *structField;

    structField = mxGetField(entryStruct, 0, fieldname);
    if (structField) {
        if (!isNumericScalar(structField)) {
            mexPrintf("Parameter %s must be a numeric 1x1.\n", fieldname);
            return 0;
        }

        *k = mxGetPr(structField)[0];
        return 1;
    }

    return 1;
}

int32_t convert_struct_logical_field_to_uint32(const mxArray* entryStruct, char* fieldname, uint32_t* k, uint32_t negate) {
    const mxArray  *structField;

    structField = mxGetField(entryStruct, 0,fieldname);
    if (structField) {
        if (!mxIsLogical(structField)) {
            mexPrintf("Parameter %s should be true or false.", fieldname);
            return 0;
        } else {
            if (negate) {
                *k = !(mxGetLogicals(structField)[0]);
            } else {
                *k = mxGetLogicals(structField)[0];
            }
            return 1;
        }
    }
    return 1;
}

int32_t convert_struct_field_to_int32(const mxArray* entryStruct, char* fieldname, int32_t* k) {
    const mxArray  *structField;

    structField = mxGetField(entryStruct, 0, fieldname);
    if (structField) {
        if (!isNumericScalar(structField)) {
            mexPrintf("Parameter %s must be a numeric 1x1.\n", fieldname);
            return 0;
        }

        *k = mxGetPr(structField)[0];
        return 1;
    }

    return 1;
}

int32_t convert_struct_field_to_value_type(const mxArray* entryStruct, char* fieldname, VALUE_TYPE* v) {
    const mxArray  *structField;

    structField = mxGetField(entryStruct, 0, fieldname);
    if (structField) {
        if (!isNumericScalar(structField)) {
            mexPrintf("Parameter %s must be a numeric 1x1.\n", fieldname);
            return 0;
        }

        *v = mxGetPr(structField)[0];
        return 1;
    }

    return 1;
}

uint32_t convert_struct_field_to_cdict(const mxArray* entryStruct
                                  , char* field_name
                                  , struct kmeans_params * prms
                                  , uint32_t conversion_type) {
    const mxArray  *structField;
    char* input_buf;

    structField = mxGetField(entryStruct, 0, field_name);
    if (structField) {
        const mxArray  *substruct;
        if (mxIsStruct(structField) && myIsScalar(structField)) {
            int no_fields;
            int i;
            char* sub_fieldname;
            substruct = structField;
            structField = NULL;
            no_fields = mxGetNumberOfFields(substruct);
            for (i = 0; i < no_fields; i++) {
                structField = mxGetFieldByNumber(substruct, 0, i);
                sub_fieldname = (char*) mxGetFieldNameByNumber(substruct, i);

                if (conversion_type == CONVERSION_NUMERIC) {
                    if (!isNumericScalar(structField)) {
                        mexPrintf("Warning: ignoring %s.%s since it needs to be a 1x1 numeric value!\n", field_name, sub_fieldname);
                        continue;
                    }

                    d_add_subfloat(&(prms->tr)
                                 , field_name
                                 , sub_fieldname
                                 , mxGetScalar(structField));
                }

                if (conversion_type == CONVERSION_STRING) {
                    if (!isCharScalar(structField)) {
                        mexPrintf("Warning: ignoring %s.%s since it needs to be a string value!\n", field_name, sub_fieldname);
                        continue;
                    }

                    input_buf = mxArrayToString(structField);
                    d_add_substring(&(prms->tr)
                                 , field_name
                                 , sub_fieldname
                                 , input_buf);
                    mxFree(input_buf);
                }
            }
        } else {
            mexPrintf("Warning: ignoring %s since it needs to be a 1x1 struct!\n", field_name);
            return 0;
        }
    }

    return 1;
}

mxArray* convert_uint64_array_to_mxarray(uint64_t* arr, uint64_t len) {
    uint64_t i;
    uint64_t* uint_element;
    mxArray* mx_element;

    mx_element = mxCreateNumericMatrix(len, 1, mxUINT64_CLASS, mxREAL);
    uint_element = (uint64_t*) mxGetData(mx_element);
    for (i = 0; i < len; i++) {
        uint_element[i] = arr[i];
    }

    return mx_element;
}

mxArray* convert_valuetype_array_to_mxarray(VALUE_TYPE* arr, uint64_t len) {
    uint64_t i;
    VALUE_TYPE* fval_element;
    mxArray* mx_element;

    mx_element = mxCreateNumericMatrix(len, 1, mxDOUBLE_CLASS, mxREAL);
    fval_element = (VALUE_TYPE*) mxGetData(mx_element);
    for (i = 0; i < len; i++) {
        fval_element[i] = arr[i];
    }

    return mx_element;
}

uint32_t load_dataset(const mxArray* input_data, struct csr_matrix **input_dataset) {
    if (isCharScalar(input_data)) {
        char* input_buf;
        int success;
        success = 1;

        input_buf = mxArrayToString(input_data);

        if (convert_libsvm_file_to_csr_matrix_wo_labels(input_buf, input_dataset)) {
            success = 0;
            goto fail_load_scalar_dataset;
        }

fail_load_scalar_dataset:
        mxFree(input_buf);
        return success;

    } else {
        mwSize dim, num, nnz;               // size of input data
        double * X;                         // the data in matlab format (len: nnz)
        mwIndex * irs;                      // specifies the row indices (len: nnz)
        mwIndex * jcs;                      // specifies how many values in each column (len: num)

        // check if input is sparse
        if(!mxIsDouble(input_data) || !mxIsSparse(input_data)) {
            mexPrintf("Wrong input. Only sparse matrices with double precision values are supported!. Try sparse(X).\n");
            return 0;
        }

        // get dimension of input parameters (assume points are stored columnwise)
        dim = mxGetM(input_data);          // number of rows of X (= dimension)
        num = mxGetN(input_data);          // number of columns of X (= number of samples)
        nnz = mxGetNzmax(input_data);      // number of nonzero entries

        if (KEY_TYPE_MAX < UINT64_MAX) {
            if (dim > (((uint64_t) KEY_TYPE_MAX) + 1)) {
                mexPrintf("Wrong input. Dimension cannot be larger than 2^32\n");
                return 0;
            }
        }

        // read input data
        X = mxGetPr(input_data);
        irs = mxGetIr(input_data);
        jcs = mxGetJc(input_data);

        *input_dataset = NULL;

        // convert from matlab csc format to csr matrix format
        convert_to_csr_matrix(input_dataset, X, irs,jcs, nnz, num, dim);
    }

    return 1;
}

uint32_t read_optional_params(struct kmeans_params * prms, const mxArray *entryStruct) {
    const mxArray  *structField;
    char* input_buf;
    int32_t i;
    int32_t no_cores;

    if (!mxIsStruct(entryStruct)) {
        return 0;
    }


    structField = mxGetField(entryStruct,0,"algorithm");
    if (structField) {
        if (!mxIsChar(structField) || !myIsScalar(structField)) {
            mexPrintf("Parameter algorithm must be a string.\n");
            return 0;
        }
        input_buf = mxArrayToString(structField);

        for (i = 0; i < NO_KMEANS_ALGOS; i++) {
            if (strcmp(input_buf, KMEANS_ALGORITHM_NAMES[i])==0) {
                prms->kmeans_algorithm_id = i;
                break;
            }
        }
        if (i == NO_KMEANS_ALGOS) {
            mexPrintf("Unknown algorithm %s.\n",input_buf);
            mxFree(input_buf);
            return 0;
        }

        mxFree(input_buf);
    }

    no_cores = -1;

    if (!convert_struct_field_to_uint32(entryStruct, "seed", (uint32_t*) &(prms->seed))) return 0;
    if (!convert_struct_field_to_int32(entryStruct, "no_cores", &no_cores)) return 0;
    if (!convert_struct_field_to_uint32(entryStruct, "max_iter", &(prms->iteration_limit))) return 0;
    if (!convert_struct_field_to_value_type(entryStruct, "tol", &(prms->tol))) return 0;
    if (!convert_struct_logical_field_to_uint32(entryStruct, "silent", &(prms->verbose), 1)) return 0;
    if (!convert_struct_logical_field_to_uint32(entryStruct, "remove_empty", &(prms->remove_empty), 0)) return 0;

    omp_set_num_threads(no_cores);

    if (prms->iteration_limit < 1) {
        mexPrintf("Iteration limit needs to be at least 1.\n");
        return 0;
    }

    if (prms-> tol <= 0) {
        mexPrintf("Tolerance needs to be larger than zero.\n");
        return 0;
    }

    structField = mxGetField(entryStruct, 0, "init");
    if (structField) {
        if (!mxIsChar(structField) || !myIsScalar(structField)) {
            mexPrintf("Parameter init must be a string.\n");
            return 0;
        }

        input_buf = mxArrayToString(structField);
        for (i = 0; i < NO_KMEANS_INITS; i++) {
            if (strcmp(input_buf, KMEANS_INIT_NAMES[i])==0) {
                prms->init_id = i;
                break;
            }
        }
        if (i == NO_KMEANS_INITS) {
            mexPrintf("Unknown initialization strategy %s.\n",input_buf);
            mxFree(input_buf);
            return 0;
        }

        mxFree(input_buf);
    }
    
    convert_struct_field_to_cdict(entryStruct,"additional_params", prms, CONVERSION_NUMERIC);
    convert_struct_field_to_cdict(entryStruct,"info", prms, CONVERSION_STRING);

    return 1;
}

void cdict_element_to_mx_element(mxArray* mx_struct, struct cdict* element) {
    uint64_t i;
    uint64_t* uint_element;
    VALUE_TYPE* fval_element;
    mxArray* mx_element;
    mxArray* mx_sub_element;

    mx_element = NULL;

    switch (element->type) {
        case DICT_I:
            mx_element = mxCreateNumericMatrix(1, 1, mxUINT64_CLASS, mxREAL);
            if (!mx_element) break;
            uint_element = (uint64_t*) mxGetData(mx_element);
            *uint_element = element->val;

            break;
        case DICT_F:
            mx_element = mxCreateNumericMatrix(1, 1, mxDOUBLE_CLASS, mxREAL);
            if (!mx_element) break;
            fval_element = (VALUE_TYPE*) mxGetData(mx_element);
            *fval_element = element->fval;
            break;

        case DICT_D:
            mx_element = create_struct(&(element->d_val));
            break;
        case DICT_DL:
            mx_element = mxCreateCellMatrix(element->val, 1);
            if (!mx_element) break;

            for (i = 0; i < element->val; i++) {
                mx_sub_element = NULL;
                mx_sub_element = create_struct(&(element->d_list[i]));
                if (mx_sub_element != NULL) mxSetCell(mx_element, i, mx_sub_element);
            }

            break;
        case DICT_FL:
            mx_element = mxCreateNumericMatrix(element->val, 1, mxDOUBLE_CLASS, mxREAL);
            if (!mx_element) break;
            fval_element = (VALUE_TYPE*) mxGetData(mx_element);
            for (i = 0; i < element->val; i++) {
                fval_element[i] = element->fval_list[i];
            }

            break;

        case DICT_IL:
            mx_element = mxCreateNumericMatrix(element->val, 1, mxUINT64_CLASS, mxREAL);
            if (!mx_element) break;
            uint_element = (uint64_t*) mxGetData(mx_element);
            for (i = 0; i < element->val; i++) {
                uint_element[i] = element->val_list[i];
            }
            break;

        case DICT_ST:
            mx_element = mxCreateString(element->sval);
            break;

        case DICT_SL:
            mx_element = mxCreateCellMatrix(element->val, 1);
            if (!mx_element) break;
            for (i = 0; i < element->val; i++) {
                mxSetCell(mx_element, i, mxCreateString(element->sval_list[i]));
            }
            break;

        default: break;
    }
    if (mx_element != NULL) mxSetField(mx_struct, 0, element->name, mx_element);
}

mxArray* create_struct(struct cdict** d) {
    struct cdict *element, *tmp;
    mxArray* mx_struct;
    int field_number;
    element = NULL;
    tmp = NULL;

    mx_struct = mxCreateStructMatrix(1, 1, 0, 0);
    if (mx_struct != NULL) {
        /* free the hash table contents */
        HASH_ITER(hh, *d, element, tmp) {
            field_number = mxAddField(mx_struct, element->name);
            if (field_number != -1) {
                cdict_element_to_mx_element(mx_struct, element);
            }
        }
    }
    return mx_struct;
}
