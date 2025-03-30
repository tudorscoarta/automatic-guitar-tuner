% Component values
R = 7.8e3; % 7.8 kOhm
C = 10e-9; % 10 nF
fc = 2041; % Cutoff frequency

% Frequency range for plotting
f = logspace(1, 5, 1000); % From 10 Hz to 100 kHz

omega = 2 * pi * f;

% Transfer functions for filters
H2 = 1 ./ sqrt(1 + (omega / (2 * pi * fc)).^2);
H4 = 1 ./ sqrt(1 + (omega / (2 * pi * fc)).^4);
H6 = 1 ./ sqrt(1 + (omega / (2 * pi * fc)).^6);

% Convert to decibels
H2_dB = 20 * log10(H2);
H4_dB = 20 * log10(H4);
H6_dB = 20 * log10(H6);

% Ideal brick-wall response
brickwall = -100 * ones(size(f));
brickwall(f <= fc) = 0;

% Plot frequency response
figure;
set(gcf, 'Color', [1 1 1])
semilogx(f, H2_dB, 'b', 'LineWidth', 1.5); hold on;
semilogx(f, H4_dB, 'r', 'LineWidth', 1.5);
semilogx(f, H6_dB, 'g', 'LineWidth', 1.5);
semilogx(f, brickwall, 'k--', 'LineWidth', 1.5);
grid on;
xlabel('Frequency (Hz)', 'FontSize', 24);
ylabel('Magnitude (dB)', 'FontSize', 24);
title('Frequency Response of Sallen-Key Filters of Order 2, 4, and 6', 'FontSize', 36);
legend('Order 2', 'Order 4', 'Order 6', 'Ideal Brickwall Response', 'FontSize', 24);
axis([10 100000 -100 10]);

% Adjust font sizes
set(gca, 'FontSize', 18);

% Save figure
set(gcf, 'Position', [100, 100, 1000, 800]);
print(gcf, 'filter_response.png', '-dpng', '-r300');
