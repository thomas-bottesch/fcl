#include "fcl_string.h"
#include <stdlib.h>
#include <string.h>

char *dupstr(const char *s) {
    char *p = malloc(strlen(s) + 1);
    if(p != NULL) strcpy(p, s);
    return p;
}
