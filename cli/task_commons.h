#ifndef TASK_COMMONS_H
#define TASK_COMMONS_H

#include "../utils/pstdint.h"

#define SUBTASK_FIT                                UINT32_C(0)
#define SUBTASK_PREDICT                            UINT32_C(1)

unsigned int parse_command_fit_predict(int argc, char *argv[], char* chosen_algorithm);

#endif