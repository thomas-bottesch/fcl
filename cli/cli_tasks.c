#include <stdlib.h>
#include "../utils/argtable3.h"

#include "cli_tasks.h"

const char *CLI_ALGORITHM_NAMES[NO_CLI_ALGOS] = {"kmeans",
                                                 "kmeans++"};
cli_function CLI_ALGORITHM_FUNCTIONS[NO_CLI_ALGOS] = {kmeans_task,
                                                      kmeanspp_task};
