#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "../utils/argtable3.h"
#include "cli_tasks.h"

unsigned int parse_command_line_task(int argc, char *argv[]) {
    struct arg_lit *help = arg_lit0(NULL,"help", "print this help and exit");
    struct arg_str *task = arg_str1(NULL,NULL,"task", "choose the clustering task: [kmeans]");
    struct arg_end *end = arg_end(20);
    unsigned int no_cli_algorithms;
    unsigned int i;

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
        printf("Usage: %s", progname);
        arg_print_syntax(stdout, argtable, "\n");
        printf("Library used for various clustering tasks.\n\n");
        printf("Choose a task e.g.:\n");
        printf("./fcl kmeans\n\n");
        printf("./fcl kmeanspp\n\n");

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

    no_cli_algorithms = sizeof(CLI_ALGORITHM_NAMES) / sizeof(CLI_ALGORITHM_NAMES[0]);

    for (i = 0; i < no_cli_algorithms; i++) {
        if (strcmp(task->sval[0], CLI_ALGORITHM_NAMES[i]) == 0) {
            return_value = i;
            goto cli_success;
        }
    }

    printf("Unknown task: %s\n\n", task->sval[0]);
    goto usage_task_params;

cli_success:
    /* deallocate each non-null entry in argtable[] */
    arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
    return return_value;
}

int main (int argc, char *argv[]) {
    unsigned int clustering_task;

    clustering_task = parse_command_line_task(argc, argv);

    if (clustering_task == CLUSTERING_TASK_KMEANS) {
        kmeans_task(argc - 1, argv + 1);
    } else if (clustering_task == CLUSTERING_TASK_KMEANSPP) {
        //kmeanspp_task(argc - 1, argv + 1);
    }

    return 0;
}
