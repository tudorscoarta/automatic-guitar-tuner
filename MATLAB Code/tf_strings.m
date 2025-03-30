clc;
clear all;
close all;

% Frequencies read for the low E string 
E_increasing_tension = [53.32, 56.74, 58.79, 61.52, 64.26, 66.99, 69.04, 71.78, 73.83, 75.88, 77.25, 77.93, 79.98, 82.03];
E_decreasing_tension = [106.64, 104.59, 103.22, 101.17, 99.12, 97.75, 95.02, 93.65, 91.60, 89.55, 87.50, 86.13, 83.40, 82.03];

% Frequencies read for the A string
A_increasing_tension = [85.45, 87.50, 89.55, 90.92, 93.65, 95.70, 97.75, 98.44, 101.17, 103.22, 104.59, 106.64, 109.37, 110.06];
A_decreasing_tension = [132.62, 131.25, 129.88, 127.83, 126.46, 124.41, 123.05, 121.00, 118.95, 116.89, 115.53, 114.16, 112.11, 110.06];

% Frequencies read for the D string
D_increasing_tension = [114.16, 115.53, 117.58, 118.95, 122.36, 124.41, 127.05, 129.70, 132.35, 135.00, 137.65, 140.30, 142.95, 145.60];
D_decreasing_tension = [172.30, 171.58, 170.85, 168.16, 166.11, 164.06, 162.01, 158.59, 157.91, 155.18, 153.12, 150.39, 147.66, 144.92];

% Frequencies read for the G string
G_increasing_tension = [155.89, 160.35, 162.9, 165.66, 167.67, 170.74, 173.59, 178.61, 181.05, 184.38, 185.81, 191.17, 193.91, 195.5];
G_decreasing_tension = [234.75, 231.67, 228.89, 226.34, 223.04, 219.63, 217.32, 213.1, 210.39, 207.5, 204.63, 202.34, 197.87, 195.54];

% Frequencies read for the B string
B_increasing_tension = [195.00, 198.43, 201.87, 205.30, 207.13, 210.55, 214.65, 219.43, 224.22, 229.00, 233.79, 237.21, 241.31, 245.41];
B_decreasing_tension = [293.50, 290.00, 286.50, 283.01, 279.59, 276.17, 272.75, 270.02, 266.60, 262.50, 258.40, 254.30, 249.51, 245.41];

% Frequencies read for the high e string 
e_increasing_tension = [287.02, 290.47, 293.89, 297.35, 299.41, 302.83, 306.93, 310.35, 313.77, 316.50, 319.92, 323.34, 327.44, 329.49];
e_decreasing_tension = [360.00, 356.70, 353.40, 351.50, 350.00, 348.50, 347.32, 346.58, 343.16, 340.43, 337.01, 334.28, 330.86, 328.12];

% Calculating transfer functions
calculate_tf(E_increasing_tension, E_decreasing_tension, 'E');
calculate_tf(A_increasing_tension, A_decreasing_tension, 'A');
calculate_tf(D_increasing_tension, D_decreasing_tension, 'D');
calculate_tf(G_increasing_tension, G_decreasing_tension, 'G');
calculate_tf(B_increasing_tension, B_decreasing_tension, 'B');
calculate_tf(e_increasing_tension, e_decreasing_tension, 'e');

% Define a function to calculate and display the transfer function
function calculate_tf(increasing_frequencies, decreasing_frequencies, string_name)
    % Step interval
    steps = 0:20:(length(increasing_frequencies)-1)*20;
    
    % Intermediate k values for increasing tension
    k_increasing = diff(increasing_frequencies) / 20;
    average_k_increasing = mean(k_increasing);
    
    % Intermediate k values for decreasing tension
    k_decreasing = diff(decreasing_frequencies) / 20;
    average_k_decreasing = mean(k_decreasing);
    
    % Results
    fprintf('For string %s:\n', string_name);
    fprintf('We have k_process_decreasing=%.4f, k_process_increasing=%.4f\n', average_k_decreasing, average_k_increasing);
    fprintf('Using controllers kp1=%.4f, kp2=%.4f\n', abs(1/average_k_decreasing), abs(1/average_k_increasing));
    fprintf('Transfer function for %s_decreasing: H = %.4f * 1/s\n', string_name, average_k_decreasing);
    fprintf('Transfer function for %s_increasing: H = %.4f * 1/s\n\n', string_name, average_k_increasing);
end
