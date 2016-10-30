#include "csr_store_matrix.h"

uint32_t store_matrix_with_label(struct csr_matrix *mtrx, int32_t* labels, int32_t static_label, char* output_path) {
    FILE *file;
    uint64_t vector_id;
    UT_string *s;
    file = fopen(output_path, "w");
    if (!file) {
        return 1;
    }

    utstring_new(s);
    for (vector_id = 0; vector_id < mtrx->sample_count; vector_id++) {
        KEY_TYPE* keys;
        VALUE_TYPE* values;
        uint64_t nnz;
        keys = mtrx->keys + mtrx->pointers[vector_id];
        values = mtrx->values + mtrx->pointers[vector_id];
        nnz = mtrx->pointers[vector_id + 1] - mtrx->pointers[vector_id];
        if (labels != NULL) {
            create_ascii_lsvm_with_label(keys, values, nnz, labels[vector_id], s);
        } else {
            create_ascii_lsvm_with_label(keys, values, nnz, static_label, s);
        }
        fprintf(file, "%s", utstring_body(s));
        utstring_clear(s);
    }
    utstring_free(s);

    fclose(file);
    return 0;
}

char* store_matrix_with_label_as_string(struct csr_matrix *mtrx, int32_t* labels, int32_t static_label) {
    uint64_t vector_id;
    UT_string *s;
    char* result_string;
    result_string = NULL;

    utstring_new(s);
    for (vector_id = 0; vector_id < mtrx->sample_count; vector_id++) {
        KEY_TYPE* keys;
        VALUE_TYPE* values;
        uint64_t nnz;
        keys = mtrx->keys + mtrx->pointers[vector_id];
        values = mtrx->values + mtrx->pointers[vector_id];
        nnz = mtrx->pointers[vector_id + 1] - mtrx->pointers[vector_id];
        create_ascii_lsvm_with_label(keys, values, nnz, static_label, s);
        if (labels != NULL) {
            create_ascii_lsvm_with_label(keys, values, nnz, labels[vector_id], s);
        } else {
            create_ascii_lsvm_with_label(keys, values, nnz, static_label, s);
        }
    }
    result_string = utstring_body(s);

    /* this frees the utstring structure but leaves the pointer intact */
    free(s);
    return result_string;
}

void create_ascii_lsvm_with_label(KEY_TYPE* keys, VALUE_TYPE* values,
                                  uint64_t nnz, int32_t label,
                                  UT_string *result_string) {
    uint64_t i;

    utstring_printf(result_string, "%" PRINTF_INT32_MODIFIER "d", label);
    for (i = 0; i < nnz; i++) {
        utstring_printf(result_string, " %" PRINTF_INT32_MODIFIER "u:%.18g", keys[i] + 1, values[i]);
    }
    utstring_printf(result_string, "\n");
}
