function dataset_fullpath = get_dataset(x)
    path_to_this_script = mfilename('fullpath');
    [global_folder,dirname,~]=fileparts(path_to_this_script);
    [global_folder,dirname,~]=fileparts(global_folder);
    matlab_examples_folder = fullfile(global_folder, dirname);
    [global_folder,dirname,~]=fileparts(matlab_examples_folder);
    dataset_folder = fullfile(global_folder, 'datasets');
    dataset_fullpath = fullfile(dataset_folder, x);
    
    if exist(dataset_fullpath, 'file') == 2
        %dataset already exists
        %fprintf('Dataset already exists: %s\n', dataset_fullpath);
    else
        fprintf('Downloading dataset from github to: %s ...', dataset_fullpath);
        url = sprintf('https://github.com/thomas-bottesch/fcl_datasets/blob/master/%s?raw=true', x);
        outfilename = urlwrite(url, dataset_fullpath);
        fprintf(' succeeded\n');
    end
end
