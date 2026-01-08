%% MATLAB
%% This script establishes a high-speed serial connection to identify, capture, and decode binary packets from a dual-sensor system. 
%% It then automatically filters out unwritten memory before visualizing the results and exporting synchronized datasets in both .MAT and .CSV formats

%% BINARY SERIAL RECEIVER & DECODER
% Description: Receives dual-sensor data, filters memory padding, and exports to CSV/MAT.

clear; clc; close all;

%% --- 1. Configuration ---
cfg.port = "COM3";
cfg.baud = 921600;
cfg.packetSize = 16;            % 6B (S1) + 6B (S2) + 4B (Time)
cfg.totalBytes = 65536 * 2 * 256; 
cfg.timeout = 15;               % Seconds to wait before partial processing

% Sensor Mode Map
modes = {'Accel','Coil'; 'Accel','Gyro'; 'Accel','Mag'; ...
         'Gyro','Mag'; 'Gyro','Coil'; 'Mag','Coil'};

try
    dev = serialport(cfg.port, cfg.baud);
    flush(dev);
catch
    error('Port %s unavailable.', cfg.port);
end

fprintf('Waiting for Mode Byte...\n');

%% --- 2. Data Acquisition ---
% Read header to identify sensors
mIdx = read(dev, 1, "uint8");
if isempty(mIdx) || mIdx > 5, error('Invalid Mode Byte.'); end

s1N = modes{mIdx + 1, 1}; s2N = modes{mIdx + 1, 2};
fprintf('>>> Mode: [%s] & [%s]. Downloading...\n', s1N, s2N);

rawData = zeros(cfg.totalBytes, 1, 'uint8'); 
ptr = 1; ticUpdate = tic; 

while ptr <= cfg.totalBytes
    if dev.NumBytesAvailable > 0
        chunk = read(dev, dev.NumBytesAvailable, "uint8");
        len = min(length(chunk), cfg.totalBytes - ptr + 1);
        rawData(ptr : ptr + len - 1) = chunk;
        ptr = ptr + len;
        ticUpdate = tic; 
        
        if mod(ptr, 1048576) < 1024 % Progress update every 1MB
            fprintf('Progress: %.1f%%\n', (ptr/cfg.totalBytes)*100);
        end
    elseif toc(ticUpdate) > cfg.timeout
        fprintf('\n[!] Timeout. Processing received data.\n');
        break; 
    end
    pause(0.01); 
end
rawData = rawData(1 : ptr-1);

%% --- 3. Decoding & Filtering ---
numPkts = floor(length(rawData) / cfg.packetSize);
pkts = reshape(rawData(1 : numPkts * cfg.packetSize), cfg.packetSize, [])';

% Extract Time (uint32) & Data (int16)
t_sec = (double(typecast(reshape(pkts(:, 13:16)', [], 1), 'uint32')) - ...
         double(typecast(pkts(1, 13:16), 'uint32'))) / 1e6;
s1 = double(reshape(typecast(reshape(pkts(:, 1:6)', [], 1), 'int16'), 3, [])');
s2 = double(reshape(typecast(reshape(pkts(:, 7:12)', [], 1), 'int16'), 3, [])');

% Smart Trim: Remove trailing 0xFF/empty memory (10x [-1] sequence)
empty = all(s1 == -1, 2) & all(s2 == -1, 2);
isSeq = conv(double(empty), ones(10, 1), 'valid') == 10;
stopIdx = find(isSeq, 1, 'first');

if ~isempty(stopIdx)
    valid = 1:(stopIdx-1);
    t_sec = t_sec(valid); s1 = s1(valid,:); s2 = s2(valid,:);
    fprintf('>>> Trimmed %d empty packets.\n', numPkts - length(valid));
end

%% --- 4. Visualization ---
clrs = [0.85 0.33 0.1; 0.47 0.67 0.19; 0 0.45 0.74];
for fig = 1:2
    name = eval(sprintf('s%dN', fig)); data = eval(sprintf('s%d', fig));
    figure('Name', name, 'Color', 'w');
    for i = 1:3
        subplot(3,1,i); plot(t_sec, data(:,i), 'Color', clrs(i,:));
        grid on; ylabel(['Axis ', num2str(i)]);
        if i==1, title(['Raw Data: ', name]); end
    end
    xlabel('Time (s)');
end

%% --- 5. Export ---
fDir = sprintf('Data_%s_%s_%s', s1N, s2N, datestr(now, 'yyyy-mm-dd_HHMM'));
mkdir(fDir);
save(fullfile(fDir, 'dataset.mat'), 't_sec', 's1', 's2', 's1N', 's2N');
writetable(table(t_sec, s1(:,1), s1(:,2), s1(:,3)), fullfile(fDir, [s1N '.csv']));
writetable(table(t_sec, s2(:,1), s2(:,2), s2(:,3)), fullfile(fDir, [s2N '.csv']));
fprintf('>>> Done. Saved to: %s\n', fDir);
