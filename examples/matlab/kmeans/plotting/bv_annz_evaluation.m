function bv_annz_evaluation()   
    addpath(fileparts(fileparts(fileparts(mfilename('fullpath')))));
    check_compile('kmeans')
    datasets.sector = get_dataset('sector.scaled');
    datasets.usps = get_dataset('usps.scaled');
    do_bv_annz_evaluation(datasets)
end

function do_bv_annz_evaluation(datasets)
    opts.silent = true;
    opts.no_cores = 1;
    opts.seed = 0;
    opts.algorithm = 'bv_kmeans';
    
    bv_annz_values = linspace(1, 100, 10) / 100.0;
    
    dataset_names = fieldnames(datasets);
    result_durations = cell(numel(dataset_names),1);
    result_bv_annz = cell(numel(dataset_names),1);
    for i = 1:numel(dataset_names)
      dataset_path = datasets.(dataset_names{i});
      for bv_annz = bv_annz_values
          additional_params.bv_annz = bv_annz;
          opts.additional_params = additional_params;
          fprintf('Executing %s for %s with bv_annz: %f\n', opts.algorithm, dataset_names{i}, bv_annz);
          [ C, tracked_params ] = fcl_kmeans_fit(dataset_path, 1000, opts);
          result_durations{i} = [result_durations{i} tracked_params.duration_kmeans];
          result_bv_annz{i} = [result_bv_annz{i} tracked_params.additional_params.bv_annz];
      end
    end
    
    figure('DefaultAxesFontSize',12,'Position', [100, 100, 800, 800]);
    cc=hsv(numel(dataset_names));
    hold on;
    for i = 1:numel(dataset_names)
        plot(result_bv_annz{i}, result_durations{i} / 1000.0, 'color', cc(i,:), 'LineWidth', 3);
    end
    hold off;
    legend(dataset_names, 'Interpreter', 'none');
    xlabel('relative block vector size');
    ylabel('time / s');
    title('Observing algorithm speed while varying block vector size');
end
