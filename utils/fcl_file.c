#include "fcl_file.h"
#include <stdio.h>

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
