#ifndef CLI_TASKS_H
#define CLI_TASKS_H

#include "../utils/pstdint.h"
#include "kmeans_task.h"
#include "kmeanspp_task.h"

#define NO_CLI_ALGOS                               UINT32_C(2)
#define CLUSTERING_TASK_KMEANS                     UINT32_C(0)
#define CLUSTERING_TASK_KMEANSPP                   UINT32_C(1)

extern const char *CLI_ALGORITHM_NAMES[NO_CLI_ALGOS];

typedef void (*cli_function) (int argc, char *argv[]);
extern cli_function CLI_ALGORITHM_FUNCTIONS[NO_CLI_ALGOS];

#endif