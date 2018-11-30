#include "fcl_file.h"
#include <stdlib.h>

uint32_t exists(const char *fname) {
    FILE *file;

    if (fname == NULL) return 0;

    file = fopen(fname, "r");
    if (file) {
        fclose(file);
        return 1;
    }
    return 0;
}

void json_dump_uint64_array_to_open_file(FILE *file,
                                         uint32_t indent,
                                         char* array_name,
                                         uint64_t no_elements,
                                         uint64_t* elements,
                                         uint32_t with_ending_comma) {
    uint64_t i;

    fprintf(file, "%*s\"%s\": [", indent, " ", array_name);
    for (i = 0; i < no_elements - 1; i++) {
        fprintf(file, "%" PRINTF_INT64_MODIFIER "u,", elements[i]);
    }
    fprintf(file, "%" PRINTF_INT64_MODIFIER "u", elements[i]);
    fprintf(file, "]");
    if (with_ending_comma) {
        fprintf(file, ",");
    }
    fprintf(file, "\n");

    return;
}
