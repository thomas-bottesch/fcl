function fcl_make_kmeans(input_args)   

    if ~exist('input_args','var') || isempty(input_args)
      action='do_not_force_recompile';
    else
      if strcmp(input_args, 'force')
        action='force_recompile';
      else
        action='do_not_force_recompile';
      end
    end

    path_to_this_script = mfilename('fullpath');
    [global_folder,dirname,~]=fileparts(path_to_this_script);
    [global_folder,mod_dirname,~]=fileparts(global_folder);
    [global_folder,dirname,~]=fileparts(global_folder);
    algorithms_folder = fullfile(global_folder, 'algorithms', mod_dirname);
    main_utilities_folder = fullfile(global_folder, 'utils');
    matlab_folder = fullfile(global_folder, dirname);
    alg_folder = fullfile(matlab_folder, mod_dirname);
    
    algorithm_files = get_full_path_c_files(algorithms_folder);
    
    % OCTAVE
    if (exist ('OCTAVE_VERSION', 'builtin'))
        page_screen_output(0);
        page_output_immediately(1);
        setenv('CFLAGS', strcat(getenv('CFLAGS'), ' -fopenmp -O2'))
        setenv('CXXFLAGS', strcat(getenv('CXXFLAGS'), ' -fopenmp -O2'))
        additional_options = {'-lgomp -g -Wall -Wno-unknown-pragmas -DEXTENSION -DOCTAVE_EXTENSION'};
        cflags_string = {''};
    else
        %MATLAB
        additional_options = {'-largeArrayDims -lut -lgomp'};
        cflags_string = {'CFLAGS="\$CFLAGS -std=c99 -g -Wall -Wno-unknown-pragmas -DEXTENSION -DMATLAB_EXTENSION -fopenmp"'};
    end
    
    addpath(alg_folder)
    items_to_compile = {'fcl_kmeans', 'fcl_kmeans_fit', 'fcl_kmeans_predict'};
    for k=1:length(items_to_compile)
        if strcmp(action, 'force_recompile') || exist(items_to_compile{k}) ~= 3
            general_files = get_utilities_full_path_c_files(main_utilities_folder, alg_folder, algorithm_files, items_to_compile{k});
            %c_file = strcat(items_to_compile{k}, '.c')
            if (exist ('OCTAVE_VERSION', 'builtin'))
                out_flags = {'--mex', '--output', fullfile(alg_folder, items_to_compile{k})};
            else
                out_flags = {'-outdir', alg_folder};
                
            end
            
            %compile([{'mex'}, cflags_string, additional_options, {fullfile(alg_folder, c_file)}, algorithm_files, general_files, out_flags]);
            compile([{'mex'}, cflags_string, additional_options, general_files, out_flags]);
            delete('*.o');
        end
    end
end

function [ general_files ] = get_utilities_full_path_c_files(utilities_folder, alg_folder, algorithm_files, item_to_compile)
    csr_matrix_folder = fullfile(utilities_folder, 'matrix', 'csr_matrix');
    vector_list_folder = fullfile(utilities_folder, 'matrix', 'vector_list');
    common_vector_folder = fullfile(utilities_folder, 'vector', 'common');
    sparse_vector_folder = fullfile(utilities_folder, 'vector', 'sparse');
    
    general_files = {};
    
    c_file = strcat(item_to_compile, '.c');
    general_files = [ general_files fullfile(alg_folder, c_file) ];
    
    if strcmp(item_to_compile, 'fcl_kmeans')
        general_files = [ general_files fullfile(utilities_folder, 'cdict.c') ];
        general_files = [ general_files fullfile(utilities_folder, 'fcl_file.c') ];
        general_files = [ general_files fullfile(utilities_folder, 'fcl_random.c') ];
        general_files = [ general_files fullfile(utilities_folder, 'fcl_string.c') ];
        general_files = [ general_files fullfile(utilities_folder, 'fcl_time.c') ];
        general_files = [ general_files fullfile(utilities_folder, 'jsmn.c') ];
        general_files = [ general_files fullfile(utilities_folder, 'fcl_logging.c') ];
        general_files = [ general_files fullfile(utilities_folder, 'clogging.c') ];
        
        general_files = [ general_files fullfile(csr_matrix_folder, 'csr_assign.c') ];
        general_files = [ general_files fullfile(csr_matrix_folder, 'csr_load_matrix.c') ];
        general_files = [ general_files fullfile(csr_matrix_folder, 'csr_math.c') ];
        general_files = [ general_files fullfile(csr_matrix_folder, 'csr_matrix.c') ];
        general_files = [ general_files fullfile(csr_matrix_folder, 'csr_store_matrix.c') ];
        general_files = [ general_files fullfile(csr_matrix_folder, 'csr_to_vector_list.c') ];
        
        general_files = [ general_files fullfile(vector_list_folder, 'vector_list_math.c') ];
        general_files = [ general_files fullfile(vector_list_folder, 'vector_list_to_csr.c') ];
        general_files = [ general_files fullfile(vector_list_folder, 'vector_list.c') ];
        
        general_files = [ general_files fullfile(common_vector_folder, 'common_vector_math.c') ];
        general_files = [ general_files fullfile(sparse_vector_folder, 'sparse_vector_math.c') ];
        
        general_files = [ general_files fullfile(alg_folder, 'fcl_kmeans_commons.c') ];
        general_files = [ general_files fullfile(alg_folder, 'mx_fcl_kmeans_fit.c') ];
        
        general_files = [ general_files algorithm_files ];
    elseif strcmp(item_to_compile, 'fcl_kmeans_fit')
        general_files = [ general_files fullfile(utilities_folder, 'cdict.c') ];
        general_files = [ general_files fullfile(utilities_folder, 'fcl_file.c') ];
        general_files = [ general_files fullfile(utilities_folder, 'fcl_random.c') ];
        general_files = [ general_files fullfile(utilities_folder, 'fcl_string.c') ];
        general_files = [ general_files fullfile(utilities_folder, 'fcl_time.c') ];
        general_files = [ general_files fullfile(utilities_folder, 'jsmn.c') ];
        general_files = [ general_files fullfile(utilities_folder, 'fcl_logging.c') ];
        general_files = [ general_files fullfile(utilities_folder, 'clogging.c') ];
        
        general_files = [ general_files fullfile(csr_matrix_folder, 'csr_assign.c') ];
        general_files = [ general_files fullfile(csr_matrix_folder, 'csr_load_matrix.c') ];
        general_files = [ general_files fullfile(csr_matrix_folder, 'csr_math.c') ];
        general_files = [ general_files fullfile(csr_matrix_folder, 'csr_matrix.c') ];
        general_files = [ general_files fullfile(csr_matrix_folder, 'csr_to_vector_list.c') ];
        
        general_files = [ general_files fullfile(vector_list_folder, 'vector_list_math.c') ];
        general_files = [ general_files fullfile(vector_list_folder, 'vector_list_to_csr.c') ];
        general_files = [ general_files fullfile(vector_list_folder, 'vector_list.c') ];
        
        general_files = [ general_files fullfile(common_vector_folder, 'common_vector_math.c') ];
        general_files = [ general_files fullfile(sparse_vector_folder, 'sparse_vector_math.c') ];
        
        general_files = [ general_files fullfile(alg_folder, 'fcl_kmeans_commons.c') ];
        general_files = [ general_files fullfile(alg_folder, 'mx_fcl_kmeans_fit.c') ];
        
        general_files = [ general_files algorithm_files ];
    elseif strcmp(item_to_compile, 'fcl_kmeans_predict')
        general_files = [ general_files fullfile(utilities_folder, 'cdict.c') ];
        general_files = [ general_files fullfile(utilities_folder, 'fcl_file.c') ];
        general_files = [ general_files fullfile(utilities_folder, 'fcl_random.c') ];
        general_files = [ general_files fullfile(utilities_folder, 'fcl_string.c') ];
        general_files = [ general_files fullfile(utilities_folder, 'fcl_time.c') ];
        general_files = [ general_files fullfile(utilities_folder, 'jsmn.c') ];
        general_files = [ general_files fullfile(utilities_folder, 'fcl_logging.c') ];
        general_files = [ general_files fullfile(utilities_folder, 'clogging.c') ];
        
        general_files = [ general_files fullfile(csr_matrix_folder, 'csr_assign.c') ];
        general_files = [ general_files fullfile(csr_matrix_folder, 'csr_load_matrix.c') ];
        general_files = [ general_files fullfile(csr_matrix_folder, 'csr_math.c') ];
        general_files = [ general_files fullfile(csr_matrix_folder, 'csr_matrix.c') ];
        general_files = [ general_files fullfile(csr_matrix_folder, 'csr_store_matrix.c') ];
        general_files = [ general_files fullfile(csr_matrix_folder, 'csr_to_vector_list.c') ];
        
        general_files = [ general_files fullfile(vector_list_folder, 'vector_list_math.c') ];
        general_files = [ general_files fullfile(vector_list_folder, 'vector_list_to_csr.c') ];
        general_files = [ general_files fullfile(vector_list_folder, 'vector_list.c') ];
        
        general_files = [ general_files fullfile(common_vector_folder, 'common_vector_math.c') ];
        general_files = [ general_files fullfile(sparse_vector_folder, 'sparse_vector_math.c') ];
        
        general_files = [ general_files fullfile(alg_folder, 'fcl_kmeans_commons.c') ];
        general_files = [ general_files fullfile(alg_folder, 'mx_fcl_kmeans_fit.c') ];
        
        general_files = [ general_files algorithm_files ];
    else
        throw(MException('Unknown item to compile', ...
        'No info what to compile for: %s',item_to_compile));
    end
    
    
end

function [ fullpath_files ] = get_full_path_c_files(path)
    files_struct = dir( fullfile(path,'*.c') );
    files_incomp = {files_struct.name}';
    
    [ rows, cols ] = size(files_incomp);
    fullpath_files = {};
    for i = 1:rows
        fullpath_files = [fullpath_files, fullfile(path, files_incomp{i})];
    end
end

function compile(cell_arr)
    fprintf('%s\n', strjoin(cell_arr))
    pause(.001);
    eval(strjoin(cell_arr))
end
