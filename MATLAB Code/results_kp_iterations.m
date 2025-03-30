clc;
clear all;
close all;

% Readings for increase/decrease for kp = 5.15
kp1_increase = {
    [62.21, 72.46, 77.93, 80.66, 81.35, 82.03],  % Low E string
    [90.23, 96.39, 103.22, 106.64, 108.01, 109.37],  % A string
    [127.83, 138.77, 144.24, 146.29],  % D string
    [177.05, 177.73, 181.84, 186.62, 190.72, 192.77, 192.77, 194.14, 194.82, 195.51],  % G string
    [228.32, 243.56, 246.09],  % B string
    [307.62, 324.02, 328.81]  % High e string
};
kp1_decrease = {
    [99.12, 93.65, 87.50, 84.77, 84.08, 83.40],  % Low E string
    [130.57, 123.73, 117.58, 114.16, 112.11, 111.43, 110.74],  % A string
    [166.80, 159.28, 153.81, 149.71, 148.34, 147.66],  % D string
    [217.38, 213.96, 210.72, 207.13, 205.08, 201.66, 198.93, 197.56, 196.87],  % G string
    [273.44, 249.24, 247.46],  % B string
    [345.90, 331.54, 330.86, 329.49]  % High e string
};

% Readings for increase/decrease for kp = 11.52
kp2_increase = {
    [64.26, 85.45, 84.08, 81.35, 81.35, 82.03],  % Low E string
    [90.23, 112.11, 112.11, 110.74],  % A string
    [125.78, 155.18, 146.97],  % D string
    [167.00, 201.46, 184, 197.33, 192.13, 195.51],  % G string
    [179.79, 261.52, 188.10, 259.42, 204.39, 247.70, 234.22, 244.04, 246.11],  % B string
    [307.62, 357.22, 312.61, 349.49, 323.49, 338.49, 326.49, 329.49]  % High e string
};
kp2_decrease = {
    [85.45, 84.08, 81.35, 81.35, 82.03],  % Low E string
    [112.11, 112.11, 110.74],  % A string
    [155.18, 146.97],  % D string
    [213.44, 188.45, 205.93, 190.42, 202.13, 193.44, 198.58, 194.33, 195.3],  % G string
    [288.48, 200.13, 276.17, 230.99, 266.25, 234.51, 261.83, 236.09, 255.43, 244.09, 246.93],  % B string
    [349.32, 315.80, 334.96, 323.66, 329.49]  % High e string
};

% Readings for individual kp increase/decrease
kpi_increase = {
    [80, 82.41],  % Low E string
    [100, 110.00],  % A string
    [131.25, 146.83],  % D string
    [189.83, 195.51],  % G string
    [226.72, 244.39, 246.09],  % B string
    [311.4, 327.3, 329.63]  % High e string
};
kpi_decrease = {
    [87.5, 82.71],  % Low E string
    [116.89, 110.74],  % A string
    [157.23, 147.66],  % D string
    [211.91, 195.51],  % G string
    [269.35, 250.20, 246.09],  % B string
    [344.06, 332.53, 329.81]  % High e string
};

% Ideal frequencies for strings
target_frequencies = [82.41, 110.00, 146.83, 196.00, 246.94, 329.63];

% String names
string_names = {'Low E string', 'A string', 'D string', 'G string', 'B string', 'High e string'};

% Plot for strings
for i = 1:6
    figure;
    hold on;
    % Values for kp increases
    plot(1:length(kp1_increase{i}), kp1_increase{i}, 'b:', 'LineWidth', 1.5, 'Marker', 's', 'MarkerFaceColor', 'b', 'MarkerEdgeColor', 'b', 'DisplayName', 'kp=5.15');
    plot(1:length(kp2_increase{i}), kp2_increase{i}, 'r:', 'LineWidth', 1.5, 'Marker', 's', 'MarkerFaceColor', 'r', 'MarkerEdgeColor', 'r', 'DisplayName', 'kp=11.52');
    plot(1:length(kpi_increase{i}), kpi_increase{i}, 'k:', 'LineWidth', 1.5, 'Marker', 's', 'MarkerFaceColor', 'k', 'MarkerEdgeColor', 'k', 'DisplayName', 'individual kp');
    
    % Values for kp decreases
    plot(1:length(kp1_decrease{i}), kp1_decrease{i}, 'b:', 'LineWidth', 1.5, 'Marker', 'o', 'MarkerFaceColor', 'b', 'MarkerEdgeColor', 'b', 'HandleVisibility', 'off');
    plot(1:length(kp2_decrease{i}), kp2_decrease{i}, 'r:', 'LineWidth', 1.5, 'Marker', 'o', 'MarkerFaceColor', 'r', 'MarkerEdgeColor', 'r', 'HandleVisibility', 'off');
    plot(1:length(kpi_decrease{i}), kpi_decrease{i}, 'k:', 'LineWidth', 1.5, 'Marker', 'o', 'MarkerFaceColor', 'k', 'MarkerEdgeColor', 'k', 'HandleVisibility', 'off');
    
    % Accepted interval of ±1 Hz
    fill([xlim, fliplr(xlim)], [target_frequencies(i) - 1, target_frequencies(i) - 1, target_frequencies(i) + 1, target_frequencies(i) + 1], 'g', 'FaceAlpha', 0.3, 'EdgeColor', 'none');
    
    xlabel('Number of iterations');
    ylabel('Frequency (Hz)');
    title(string_names{i});
    legend('show');
    grid on;
    set(gca, 'FontSize', 18);
    hold off;
end
