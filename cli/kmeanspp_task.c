#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include "../algorithms/kmeans/kmeans_control.h"
#include "../utils/matrix/csr_matrix/csr_load_matrix.h"
#include "../utils/matrix/csr_matrix/csr_store_matrix.h"
#include "../utils/matrix/csr_matrix/csr_assign.h"
#include "../utils/fcl_time.h"
#include "../utils/fcl_file.h"
#include "../utils/fcl_string.h"
#include "../utils/fcl_random.h"
#include "../utils/fcl_logging.h"
#include "../utils/argtable3.h"

#include "kmeans_task.h"
#include "kmeans_task_commons.h"
#include "task_commons.h"

struct kmeans_params parse_kmeanspp_fit_params(int argc, char *argv[],
                                             struct csr_matrix **input_dataset,
                                             char** path_model_file,
                                             char** path_tracking_params) {
    struct arg_lit *help = arg_lit0(NULL,"help", "print this help and exit");
    struct arg_str *algorithm = arg_str0(NULL,"algorithm","<name>", "choose the k-means algorithm_id: (default = bv_kmeans)");
    struct arg_int *cluster_count = arg_int0(NULL,"no_clusters","<k>", "number of clusters to generate (default=10)");
    struct arg_int *random_seed = arg_int0(NULL,"seed","<random_seed>", "the random seed to generate different starting positions (default=1)");
    struct arg_int *no_cores = arg_int0(NULL,"no_cores","<no_cores>", "the number of cores to use if compiled with openmp (uses all cores with -1 = default)");
    struct arg_int *iterations = arg_int0(NULL,"iterations","<iterations>", "the mamimum number of iterations (default=1000)");
    struct arg_dbl *tol = arg_dbl0(NULL, "tolerance","<tolerance>" , "if objective is less than this, the algorithm converges");
    struct arg_lit *silent = arg_lit0(NULL, "silent", "turn off verbosity (default=false)");
    struct arg_lit *remove_empty = arg_lit0(NULL, "remove_empty", "remove empty clusters from result (resulting no_clusters will most likely be less than requested no_clusters)");
    struct arg_file *input_dataset_file = arg_file1(NULL, NULL, "file_input_dataset", "Input dataset in libsvm format");
    struct arg_file *model_file = arg_file0(NULL, "file_model", "<path>", "Path, the model should be saved to when fitting / loaded from when predicting. (mandatory if predicting)");
    struct arg_str *kmeans_init = arg_str0(NULL,"init","<name>", "choose initialization strategy: (default = random)");
    struct arg_rex *add_params1 = arg_rexn(NULL, "param", "[\\w]+:[-+]?([0-9]*[.])?[0-9]+([eE][-+]?[0-9]+)?", NULL , 0, 100, 0, "Modify internal algorithm params.");
    struct arg_rem *add_params2 = arg_rem(NULL,                                            "e.g. --param bv_size:0.3 --param bv_enable:1 --param new_param:1");
    struct arg_rex *add_info1 = arg_rexn(NULL, "info", "[\\w]+:[^\"]*", NULL , 0, 100, 0, "Add additional information to tracking param file.");
    struct arg_rem *add_info2 = arg_rem(NULL,                                            "e.g. --info dataset_name:webcrawl --info dataset_id:crwl_1");
    struct arg_rem *add_info3 = arg_rem(NULL,                                            "e.g. --info \"comment: Dataset was sampled for this Experiment\"");
    struct arg_file *tracking_param_file = arg_file0(NULL, "file_tracking_params", "<path>", "Output tracked params from algorithm to file in json format.");
    struct arg_file *input_vectors_file = arg_file0(NULL, "file_input_vectors", "<path>", "Input vectors in libsvm format, e.g. for PCA vectors");

    struct arg_end *end = arg_end(20);
    struct kmeans_params prms;
    int no_kmeans_algorithms;
    int no_kmeans_inits;
    KEY_TYPE i;

    int* labels;

    void *argtable[100];


    int nerrors;
    int args_set;
    char *progname;
    char algorithm_buffers[sizeof(KMEANS_ALGORITHM_NAMES) / sizeof(KMEANS_ALGORITHM_NAMES[0])][1000];
    char init_buffers[sizeof(KMEANS_INIT_NAMES) / sizeof(KMEANS_INIT_NAMES[0])][1000];
    args_set = 0;

    no_kmeans_algorithms = sizeof(KMEANS_ALGORITHM_NAMES) / sizeof(KMEANS_ALGORITHM_NAMES[0]);
    no_kmeans_inits = sizeof(KMEANS_INIT_NAMES) / sizeof(KMEANS_INIT_NAMES[0]);

    argtable[args_set] = input_dataset_file; args_set++;
    argtable[args_set] = algorithm; args_set++;

    for (i = 0; i < no_kmeans_algorithms; i++) {
        sprintf(algorithm_buffers[i], "%-28s  %s", KMEANS_ALGORITHM_NAMES[i], KMEANS_ALGORITHM_DESCRIPTION[i]);
        argtable[args_set] = arg_rem(NULL, algorithm_buffers[i]);
        args_set++;
    }

    argtable[args_set] = kmeans_init; args_set++;

    for (i = 0; i < no_kmeans_inits; i++) {
        sprintf(init_buffers[i], "%-28s  %s", KMEANS_INIT_NAMES[i], KMEANS_INIT_DESCRIPTION[i]);
        argtable[args_set] = arg_rem(NULL, init_buffers[i]);
        args_set++;
    }

    argtable[args_set] = no_cores; args_set++;
    argtable[args_set] = cluster_count; args_set++;
    argtable[args_set] = random_seed; args_set++;
    argtable[args_set] = iterations; args_set++;
    argtable[args_set] = tol; args_set++;
    argtable[args_set] = silent; args_set++;
    argtable[args_set] = remove_empty; args_set++;
    argtable[args_set] = model_file; args_set++;
    argtable[args_set] = input_vectors_file; args_set++;
    argtable[args_set] = add_params1; args_set++;
    argtable[args_set] = add_params2; args_set++;
    argtable[args_set] = add_info1; args_set++;
    argtable[args_set] = add_info2; args_set++;
    argtable[args_set] = add_info3; args_set++;
    argtable[args_set] = tracking_param_file; args_set++;
    argtable[args_set] = help; args_set++;
    argtable[args_set] = end; args_set++;

    initialize_random_generator();

    /* set default parameters */
    cluster_count->ival[0] = 10;
    random_seed->ival[0] = (int) rand();
    no_cores->ival[0] = -1;
    iterations->ival[0] = 1000;
    tol->dval[0] = 1e-6;
    model_file->filename[0] = NULL;
    input_vectors_file->filename[0] = NULL;

    progname = "fcl.exe";

    if (arg_nullcheck(argtable) != 0) {
        /* NULL entries were detected, some allocations must have failed */
        printf("%s: insufficient memory\n",progname);
        exit(1);
    }

    nerrors = arg_parse(argc,argv,argtable);

    /* special case: '--help' takes precedence over error reporting */
    if (help->count > 0) {
usage_kmeans_params:
        printf("Usage: %s", progname);
        add_params1->hdr.datatype = "<param_name:value>";
        add_info1->hdr.datatype = "<param_name:value>";
        arg_print_syntax(stdout, argtable, "\n");
        printf("K-Means algorithm.\n\n");
        printf("Choose one of the following tasks e.g.:\n");
        printf("1. fit a kmeans model (without storing the model file) : ./fcl kmeans fit <input_dataset>\n\n");
        printf("2. fit a kmeans model (and store the model file) : ./fcl kmeans fit <input_dataset> --file_model <output_model_path>\n\n");

        printf("Parsing options:\n");
        arg_print_glossary(stdout, argtable, "  %-29s %s\n");
        exit(0);
    }

    /* If the parser returned any errors then display them and exit */
    if (nerrors > 0) {
        /* Display the error details contained in the arg_end struct.*/
        arg_print_errors(stdout, end, progname);
        printf("\n");
        goto usage_kmeans_params;
    }

    if (!exists(input_dataset_file->filename[0])) {
        printf("Unable to open input_dataset_file: %s\n\n", input_dataset_file->filename[0]);
        goto usage_kmeans_params;
    }

    if ((input_vectors_file->filename[0] != NULL)
        && !exists(input_vectors_file->filename[0])) {
        printf("Unable to open input_vectors_file: %s\n\n", input_vectors_file->filename[0]);
        goto usage_kmeans_params;
    }

    if (cluster_count->ival[0] < 1) {
        printf("k needs to be at least one. Given: %d\n\n", cluster_count->ival[0]);
        goto usage_kmeans_params;
    }

    if (random_seed->ival[0] < 0) {
        printf("seed needs to be at least zero. Given: %d\n\n", random_seed->ival[0]);
        goto usage_kmeans_params;
    }

    if (no_cores->ival[0] < -1 || no_cores->ival[0]  == 0) {
        printf("no_cores needs to be -1 or > 0. Given: %d\n\n", no_cores->ival[0]);
        goto usage_kmeans_params;
    }

    if (iterations->ival[0] <= 0) {
        printf("iterations needs to be >= 1. Given: %d\n\n", iterations->ival[0]);
        goto usage_kmeans_params;
    }

    if (tol->dval[0] <= 0) {
        printf("tolerance needs to be >= 0. Given: %f\n\n", tol->dval[0]);
        goto usage_kmeans_params;
    }


    prms.kmeans_algorithm_id = ALGORITHM_BV_KMEANS;
    if (algorithm->count > 0) {
        for (i = 0; i < no_kmeans_algorithms; i++) {
            if (strcmp(algorithm->sval[0], KMEANS_ALGORITHM_NAMES[i])==0) {
                prms.kmeans_algorithm_id = i;
                break;
            }
        }
        if (i == no_kmeans_algorithms) {
            printf("Unknown algorithm %s\n\n", algorithm->sval[0]);
            goto usage_kmeans_params;
        }
    }

    prms.init_id = KMEANS_INIT_RANDOM;
    if (kmeans_init->count > 0) {
        for (i = 0; i < no_kmeans_inits; i++) {
            if (strcmp(kmeans_init->sval[0], KMEANS_INIT_NAMES[i])==0) {
                prms.init_id = i;
                break;
            }
        }
        if (i == no_kmeans_inits) {
            printf("Unknown kmeans init %s\n\n", kmeans_init->sval[0]);
            goto usage_kmeans_params;
        }
    }

    prms.tr = NULL;

    if (add_params1->count > 0) {
        KEY_TYPE i;

        for (i = 0; i < add_params1->count; i++) {
            char* _copy;
            _copy = dupstr(add_params1->sval[i]);
            add_additional_param_float(&prms, _copy);
            free_null(_copy);
        }
    }

    if (add_info1->count > 0) {
        KEY_TYPE i;

        for (i = 0; i < add_info1->count; i++) {
            char* _copy;
            _copy = dupstr(add_info1->sval[i]);
            add_additional_info_param(&prms, _copy);
            free_null(_copy);
        }
    }


    prms.no_clusters = cluster_count->ival[0];
    prms.seed = (unsigned int) random_seed->ival[0];
    prms.iteration_limit = iterations->ival[0];
    prms.tol = tol->dval[0];
    prms.verbose = silent->count == 0;
    prms.remove_empty = remove_empty->count > 0;
    prms.stop = 0;
    prms.ext_vects = NULL;

    if (model_file->filename[0] != NULL) {
        *path_model_file = dupstr(model_file->filename[0]);
    } else {
        *path_model_file = NULL;
    }

    if (tracking_param_file->count > 0 && tracking_param_file->filename[0] != NULL) {
        *path_tracking_params = dupstr(tracking_param_file->filename[0]);
    } else {
        *path_tracking_params = NULL;
    }

    if (prms.verbose) LOG_INFO("loading data %s k=%" PRINTF_INT32_MODIFIER "u seed=%" PRINTF_INT32_MODIFIER "u algorithm=%s init=%s no_cores=%" PRINTF_INT32_MODIFIER "d"
                            , input_dataset_file->filename[0], prms.no_clusters
                            , prms.seed
                            , KMEANS_ALGORITHM_NAMES[prms.kmeans_algorithm_id]
                            , KMEANS_INIT_NAMES[prms.init_id]
                            , no_cores->ival[0]);

    labels = NULL;
    if (convert_libsvm_file_to_csr_matrix(input_dataset_file->filename[0], input_dataset, &labels)) {
        printf("unable to load input data / invalid libsvm or file does not exist!\n\n");
        goto usage_kmeans_params;
    }
    free(labels);

    if (input_vectors_file->filename[0] != NULL) {
        labels = NULL;
        if (convert_libsvm_file_to_csr_matrix(input_vectors_file->filename[0], &(prms.ext_vects), &labels)) {
            printf("unable to load input vectors / invalid libsvm or file does not exist!\n\n");
            goto usage_kmeans_params;
        }
        free(labels);
    }

    if (prms.verbose) LOG_INFO("data loaded");


    if (no_cores->ival[0] > 0) {
        omp_set_num_threads(no_cores->ival[0]);
    }

    /* deallocate each non-null entry in argtable[] */
    arg_freetable(argtable, args_set);

    return prms;
}

void parse_kmeanspp_predict_params(int argc, char *argv[], struct csr_matrix **input_dataset, struct csr_matrix **model, char** path_prediction_result, KEY_TYPE* verbose) {
    struct arg_lit *help = arg_lit0(NULL,"help", "print this help and exit");
    struct arg_int *no_cores = arg_int0(NULL,"no_cores","<no_cores>", "the number of cores to use if compiled with openmp (uses all cores with -1 = default)");
    struct arg_lit *silent = arg_lit0(NULL, "silent", "turn off verbosity (default=false)");
    struct arg_file *input_dataset_file = arg_file1(NULL, NULL, "input_dataset", "Input dataset in libsvm format");
    struct arg_file *model_file = arg_file1(NULL, NULL, "input_model", "Path, the model should be loaded from when predicting.");
    struct arg_file *prediction_result_file = arg_file1(NULL, NULL, "output_prediction", "Path, to store the prediction result.");
    struct arg_rem  *prediction_result_file1 = arg_rem(NULL,                                 "Every line of the output file corresponds to the input file.");
    struct arg_rem  *prediction_result_file2 = arg_rem(NULL,                                 "Format:");
    struct arg_rem  *prediction_result_file3 = arg_rem(NULL,                                 "<closest_cluster_id>, <distance_to_closest_cluster>\\n");
    struct arg_end *end = arg_end(20);

    void *argtable[10];

    int nerrors;
    char *progname;

    argtable[0] = help;
    argtable[1] = no_cores;
    argtable[2] = silent;
    argtable[3] = input_dataset_file;
    argtable[4] = model_file;
    argtable[5] = prediction_result_file;
    argtable[6] = prediction_result_file1;
    argtable[7] = prediction_result_file2;
    argtable[8] = prediction_result_file3;
    argtable[9] = end;

    /* set default parameters */
    no_cores->ival[0] = -1;
    model_file->filename[0] = NULL;
    prediction_result_file->filename[0] = NULL;
    *path_prediction_result = NULL;

    progname = "fcl.exe";

    if (arg_nullcheck(argtable) != 0) {
        /* NULL entries were detected, some allocations must have failed */
        printf("%s: insufficient memory\n",progname);
        exit(1);
    }

    nerrors = arg_parse(argc,argv,argtable);

    /* special case: '--help' takes precedence over error reporting */
    if (help->count > 0) {
usage_assign_params:
        printf("Usage: %s kmeans predict", progname);
        arg_print_syntax(stdout, argtable, "\n");
        printf("K-Means.\n\n");

        printf("Parsing options:\n");
        arg_print_glossary(stdout, argtable, "  %-30s %s\n");
        exit(0);
    }

    /* If the parser returned any errors then display them and exit */
    if (nerrors > 0) {
        /* Display the error details contained in the arg_end struct.*/
        arg_print_errors(stdout, end, progname);
        printf("\n");
        goto usage_assign_params;
    }

    if (!exists(input_dataset_file->filename[0])) {
        printf("Unable to open input_dataset_file: %s\n\n", input_dataset_file->filename[0]);
        goto usage_assign_params;
    }

    if (no_cores->ival[0] < -1 || no_cores->ival[0]  == 0) {
        printf("no_cores needs to be -1 or > 0. Given: %d\n\n", no_cores->ival[0]);
        goto usage_assign_params;
    }

    *verbose = silent->count == 0;

    if (!exists(model_file->filename[0])) {
        printf("Unable to open model file: %s\n\n", model_file->filename[0]);
        goto usage_assign_params;
    }

    *path_prediction_result = dupstr(prediction_result_file->filename[0]);

    if (*verbose) LOG_INFO("loading input data and model");
    convert_libsvm_file_to_csr_matrix_wo_labels(input_dataset_file->filename[0], input_dataset);
    convert_libsvm_file_to_csr_matrix_wo_labels(model_file->filename[0], model);
    if (*verbose) LOG_INFO("input data and model loaded");

    if (no_cores->ival[0] > 0) {
        omp_set_num_threads(no_cores->ival[0]);
    }

    /* deallocate each non-null entry in argtable[] */
    arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
}

void kmeanspp_task(int argc, char *argv[]) {
    unsigned int subtask;

    char* path_model_file;
    char* path_tracking_params;
    char* path_prediction_file;

    struct csr_matrix *input_dataset;
    struct csr_matrix *clusters;
    struct kmeans_params prms;

    subtask = parse_command_fit_predict(argc, argv, "kmeanspp");
    clusters = NULL;
    path_prediction_file = NULL;

    if (subtask == SUBTASK_FIT) {
        prms = parse_kmeanspp_fit_params(argc - 1, argv + 1, &input_dataset, &path_model_file, &path_tracking_params);

        /* fit */
        clusters = KMEANS_ALGORITHM_FUNCTIONS[prms.kmeans_algorithm_id](input_dataset, &prms);

        if (path_model_file != NULL) {
            if (store_matrix_with_label(clusters, NULL, 1, path_model_file)) {
                /* some error happened while opening file */
                if (prms.verbose) LOG_ERROR("Unable to open model file: %s", path_model_file);
            } else {
                if (prms.verbose) LOG_INFO("Model file successfully written to: %s", path_model_file);
            }
        }

        if (path_tracking_params != NULL) {
            FILE* tracking_param_file;

            tracking_param_file = fopen(path_tracking_params, "wb");
            if (!tracking_param_file) {
                if (prms.verbose) LOG_ERROR("Unable to open tracking param file: %s", path_tracking_params);
            } else {
                dump_dict_as_json_to_file(&(prms.tr), tracking_param_file);
                if (prms.verbose) LOG_INFO("Tracked params successfully written to: %s", path_tracking_params);
                fclose(tracking_param_file);
            }

        }

        free_null(path_model_file);
        free_null(path_tracking_params);
        free_cdict(&(prms.tr));
        if (prms.ext_vects != NULL) {
            free_csr_matrix(prms.ext_vects);
            free(prms.ext_vects);
        }

    }
    if (subtask == SUBTASK_PREDICT) {
        /* predict */
        struct assign_result res;
        KEY_TYPE verbose;
        KEY_TYPE stop;

        /* load model */
        verbose = 1;
        stop = 0;
        parse_kmeanspp_predict_params(argc - 1, argv + 1, &input_dataset, &clusters, &path_prediction_file, &verbose);

        if (verbose) LOG_INFO("Started assigning\n");
        res = assign(input_dataset, clusters, &stop);

        if (store_assign_result(&res, path_prediction_file)) {
            if (verbose) LOG_ERROR("Error while opening predict file: %s\n", path_prediction_file);
        } else {
            if (verbose) LOG_INFO("Predictions successfully written\n");
        }

        free_assign_result(&res);
        free_null(path_prediction_file);
    }

    free_csr_matrix(clusters);
    free_null(clusters);

    free_csr_matrix(input_dataset);
    free(input_dataset);
}
