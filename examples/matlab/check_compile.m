function check_compile(target_lib, action)

    if ~exist('action','var') || isempty(action)
      action='do_not_force_recompile';
    end

    path_to_this_script = mfilename('fullpath');
    [global_folder,dirname,~]=fileparts(path_to_this_script);
    [global_folder,dirname,~]=fileparts(global_folder);
    [global_folder,dirname,~]=fileparts(global_folder);
    target_folder = fullfile(global_folder, 'matlab', target_lib);
    addpath(target_folder);
    target_func = str2func(sprintf('fcl_make_%s', target_lib));
    if (strcmp(action, 'force_recompile'))
        target_func('force')
    else
        target_func()
    end
    rehash
end
