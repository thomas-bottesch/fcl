function varying_k_evaluation()   
    addpath(fileparts(fileparts(fileparts(mfilename('fullpath')))));
    check_compile('kmeans')
    datasets.sector = get_dataset('sector.scaled');
    datasets.usps = get_dataset('usps.scaled');
    do_varying_k_evaluation(datasets)
end

function do_varying_k_evaluation(datasets)
    opts.silent = true;
    opts.no_cores = 1;
    opts.seed = 0;
    opts.algorithm = 'kmeans_optimized';
    
    k_values = [floor(linspace(2, 100, 5)) floor(linspace(200, 1000, 5))];
    
    dataset_names = fieldnames(datasets);
    result_durations = cell(numel(dataset_names),1);
    result_calcs = cell(numel(dataset_names),1);
    for i = 1:numel(dataset_names)
      dataset_path = datasets.(dataset_names{i});
      for k = k_values
          additional_params.bv_annz = 0.3;
          opts.additional_params = additional_params;
          fprintf('Executing %s for %s with k=%i (bv_annz: %f)\n', opts.algorithm, dataset_names{i}, k, additional_params.bv_annz);
          [ C, tracked_params ] = fcl_kmeans_fit(dataset_path, k, opts);
          result_durations{i} = [result_durations{i} tracked_params.duration_kmeans];
          result_calcs{i} = [result_calcs{i} sum(tracked_params.iteration_bv_calcs_success) / (sum(tracked_params.iteration_bv_calcs_success) + sum(tracked_params.iteration_full_distance_calcs))];
      end
    end
    
    figure('DefaultAxesFontSize',12,'Position', [100, 100, 800, 800]);
    cc=hsv(numel(dataset_names));
    hold on;
    for i = 1:numel(dataset_names)
        plot(k_values, result_calcs{i} * 100, 'color', cc(i,:), 'LineWidth', 3);
    end
    hold off;
    legend(dataset_names, 'Interpreter', 'none');
    xlabel('number of clusters');
    ylabel('avoided full distance calculations (percent)');
    title('Varying k and observing avoided full distance calculations (bv annz = 0.3)');
end
