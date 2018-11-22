#include "init_params.h"
#include <stdlib.h>
#include "../../utils/fcl_file.h"
#include "../../utils/global_defs.h"
#include "../../utils/fcl_logging.h"

void write_initialization_params_file(const char* fname,
                                      struct initialization_params* initprms) {
    FILE *file;
    uint64_t i;
    uint32_t indent, with_ending_comma;
    LOG_INFO("Inside write_initialization_params_file.");
    if (initprms == NULL) return;
    if (initprms->len_assignments == 0) return;
    if (initprms->assignments == NULL) return;
    if (initprms->len_initial_cluster_samples == 0) return;
    if (initprms->initial_cluster_samples == NULL) return;
    LOG_INFO("Writing file.");
    indent = 4;
    with_ending_comma = 1;
    file = fopen(fname, "w");
    fprintf(file, "{\n");
    json_dump_uint64_array_to_open_file(file,
                                          indent,
                                          "initial_cluster_samples",
                                          initprms->len_initial_cluster_samples,
                                          initprms->initial_cluster_samples,
                                          with_ending_comma);

    with_ending_comma = 0;
    json_dump_uint64_array_to_open_file(file,
                                          indent,
                                          "assignments",
                                          initprms->len_assignments,
                                          initprms->assignments,
                                          with_ending_comma);
    fprintf(file, "}\n");
    fclose(file);
}

void read_initialization_params_file(const char* fname,
                                     struct initialization_params** initprms) {
    FILE *file;
    *initprms =  (struct initialization_params*) calloc(1, sizeof(struct initialization_params));

    file = fopen(fname, "r");

    fclose(file);
    return;
}

void free_init_params(struct initialization_params* initprms) {
    if (initprms != NULL) {
        free_null(initprms->assignments);
        free_null(initprms->initial_cluster_samples);
    }
}
