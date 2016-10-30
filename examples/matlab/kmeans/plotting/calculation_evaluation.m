function calculation_evaluation()
    addpath(fileparts(fileparts(fileparts(mfilename('fullpath')))));
    dataset_path = get_dataset('sector.scaled');
    check_compile('kmeans')
    opts.silent = true;
    opts.no_cores = 1;
    opts.seed = 0;
    
    algorithms = {'kmeans_optimized', 'yinyang'};
    results = struct();
    for k=1:length(algorithms)
        opts.algorithm = algorithms{k};
        fprintf('Executing k-means with algorithm: %s\n', algorithms{k});
        [ C, tracked_params ] = fcl_kmeans_fit(dataset_path, 300, opts);
        results.(algorithms{k}) = tracked_params;
    end

    figure('DefaultAxesFontSize',12,'Position', [100, 100, 1200, 500]);
    subplot(1,7,1);
    create_subplot_bars(results)
    subplot(1,7,2:4);
    create_subplot_full_distance_calcs(results);
    subplot(1,7,5:7);
    create_subplot_iteration_duration(results);
end

function create_subplot_bars(results)
  %
  fields = fieldnames(results);
  durations = zeros(numel(fields), 1);
  for i = 1:numel(fields)
    params = results.(fields{i});
    durations(i) = params.duration_kmeans / 1000.0;
  end
  
  bar(1:numel(fields), durations)
  set(gca, 'XTick', 1:numel(fields), 'XTickLabel', fields);
  %ax = gca;
  %ax.XTickLabelRotation = 45;
  ylabel('time / s');
  title('Overall duration');
end

function create_subplot_full_distance_calcs(results)
  
  fields = fieldnames(results);
  % create color map
  cc=hsv(numel(fields));
  hold on;
  for i = 1:numel(fields)
    params = results.(fields{i});
    full_distance_calcs = params.iteration_full_distance_calcs;
    
    if isfield(params, 'iteration_bv_calcs')
        full_distance_calcs = full_distance_calcs + (params.iteration_bv_calcs * params.additional_params.bv_annz);
    end
    
    plot(1:params.no_iterations, full_distance_calcs, 'color', cc(i,:), 'LineWidth', 3);
  end
  hold off;
  legend(fields, 'Interpreter', 'none');
  xlabel('iteration');
  ylabel('full distance calculations');
  title('Full distance calculations per iteration');
end

function create_subplot_iteration_duration(results)
  
  fields = fieldnames(results);
  % create color map
  cc=hsv(numel(fields));
  hold on;
  for i = 1:numel(fields)
    params = results.(fields{i});
    plot(1:params.no_iterations, params.iteration_durations / 1000, 'color', cc(i,:), 'LineWidth', 3);
  end
  hold off;
  legend(fields, 'Interpreter', 'none');
  xlabel('iteration');
  ylabel('time / s');
  title('Duration of every iter');
end
