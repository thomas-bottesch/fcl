#include "fcl_file.h"
#include <stdio.h>
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

void read_uint64_array_from_file(const char *fname,
                                 uint64_t* no_elements,
                                 uint64_t** elements) {
    FILE *file;
    uint32_t expect_number;
    uint64_t i;
    char c;

    *no_elements = 0;
    *elements = NULL;
    expect_number = 1;

    if (!exists(fname)) {
        return;
    }

    file = fopen(fname, "r");

    do {
          c = fgetc(file);
          if (c == EOF) break;

          if (c == ',') {
              if (expect_number) goto error_invalid_format;
              expect_number = 1;
              (*no_elements)++;
          } else {
              if (!isdigit(c)) goto error_invalid_format;
              expect_number = 0;
          }
    } while (c != EOF);

    (*no_elements)++;

    if (*no_elements == 1) goto error_invalid_format;

    /* this happens if the last character is a comma */
    if (expect_number) goto error_invalid_format;

    *elements = (uint64_t*) calloc((size_t) no_elements, sizeof(uint64_t));

    fseek(file, 0, SEEK_SET);

    while(i < *no_elements) {
        if (fscanf(file, "%" PRINTF_INT64_MODIFIER "u,", ((*elements) + i)) != 1) {
            goto error_invalid_format;
        }
        i++;
    }

    fclose(file);
    return;

error_invalid_format:
    *no_elements = 0;
    if (*elements != NULL) {
        free(*elements);
        *elements = NULL;
    }
}
