#include <stdlib.h>
#include <string.h>
#include "../utils/argtable3.h"

#include "task_commons.h"

unsigned int parse_command_fit_predict(int argc, char *argv[], char* chosen_algorithm) {
    struct arg_lit *help = arg_lit0(NULL,"help", "print this help and exit");
    struct arg_str *task = arg_str1(NULL,NULL,"subtask", "choose subtask: [ fit | predict ]");
    struct arg_end *end = arg_end(20);
    void *argtable[3];

    unsigned int return_value;
    int nerrors;
    char *progname;

    argtable[0] = help;
    argtable[1] = task;
    argtable[2] = end;


    return_value = 0;
    progname = "fcl.exe";

    if (arg_nullcheck(argtable) != 0) {
        /* NULL entries were detected, some allocations must have failed */
        printf("%s: insufficient memory\n",progname);
        exit(1);
    }
    if (argc > 2) argc = 2;
    nerrors = arg_parse(argc,argv,argtable);

    /* special case: '--help' takes precedence over error reporting */
    if (help->count > 0) {
usage_task_params:
        printf("Usage: %s %s", progname, chosen_algorithm);
        arg_print_syntax(stdout, argtable, "\n");
        printf("Choose a subtask for %s e.g.:\n", chosen_algorithm);
        printf("./fcl %s fit\n\n", chosen_algorithm);

        printf("Parsing options:\n");
        arg_print_glossary(stdout, argtable, "  %-25s %s\n");
        exit(0);
    }

    /* If the parser returned any errors then display them and exit */
    if (nerrors > 0) {
        /* Display the error details contained in the arg_end struct.*/
        arg_print_errors(stdout, end, progname);
        printf("\n");
        goto usage_task_params;
    }

    if (strcmp(task->sval[0], "fit") == 0) {
        return_value = SUBTASK_FIT;
    } else if (strcmp(task->sval[0], "predict") == 0) {
        return_value = SUBTASK_PREDICT;
    } else {
        printf("Unknown %s subtask: %s\n\n", chosen_algorithm, task->sval[0]);
        goto usage_task_params;
    }

    /* deallocate each non-null entry in argtable[] */
    arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
    return return_value;
}
