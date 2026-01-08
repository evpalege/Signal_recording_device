%% RECORDING_DEVICE_MATLAB_PLOTTER

clear; clc; close all;

%% --- 1. Configuration & Connection ---
cfg.port = "COM3";          % Target COM port
cfg.baud = 921600;          % Baud rate (must match MCU)
cfg.chipPages = 65536;      
cfg.bytesPerPage = 256;
cfg.packetSize = 16;        % 6B (S1) + 6B (S2) + 4B (Time)
cfg.totalBytes = cfg.chipPages * 2 * cfg.bytesPerPage; 
cfg.timeout = 15;           % Connection timeout in seconds

% Sensor mode mapping
sensorMap = {'Accel','Coil'; 'Accel','Gyro'; 'Accel','Mag'; ...
             'Gyro','Mag'; 'Gyro','Coil'; 'Mag','Coil'};

try
    dev = serialport(cfg.port, cfg.baud);
    flush(dev);
catch
    error('Failed to open %s. Check connection or Serial Monitor.', cfg.port);
end

fprintf('Waiting for Mode Byte...\n');

%% --- 2. Data Acquisition ---
% Read header byte to determine sensor types
modeIdx = read(dev, 1, "uint8");
if isempty(modeIdx) || modeIdx > 5, error('Invalid Mode Byte received.'); end

s1_Name = sensorMap{modeIdx + 1, 1};
s2_Name = sensorMap{modeIdx + 1, 2};
fprintf('>>> Mode: [%s] & [%s]. Starting download...\n', s1_Name, s2_Name);

% Preallocate buffer
rawData = zeros(cfg.totalBytes, 1, 'uint8'); 
ptr = 1; 
lastUpdate = tic; 

while ptr <= cfg.totalBytes
    if dev.NumBytesAvailable > 0
        chunk = read(dev, dev.NumBytesAvailable, "uint8");
        len = min(length(chunk), cfg.totalBytes - ptr + 1);
        rawData(ptr : ptr + len - 1) = chunk;
        ptr = ptr + len;
        lastUpdate = tic; 
        
        if mod(ptr, round(cfg.totalBytes/10)) < 512
            fprintf('Progress: %.1f%% (%d bytes)\n', (ptr/cfg.totalBytes)*100, ptr-1);
        end
    elseif toc(lastUpdate) > cfg.timeout
        fprintf('\n[!] Timeout reached. Processing partial data.\n');
        break; 
    end
    pause(0.01); 
end

rawData = rawData(1 : ptr-1);

%% --- 3. Decoding & Smart Filtering ---
numPackets = floor(length(rawData) / cfg.packetSize);
if numPackets == 0, error('No data packets received.'); end

% Reshape raw byte stream into packet matrix
pkts = reshape(rawData(1 : numPackets * cfg.packetSize), cfg.packetSize, [])';

% Extract Time (4 bytes, uint32) -> convert to seconds
t_raw = typecast(reshape(pkts(:, 13:16)', [], 1), 'uint32');
t_sec = (double(t_raw) - double(t_raw(1))) / 1e6;

% Extract Sensor Data (3 axes each, int16)
s1_raw = double(reshape(typecast(reshape(pkts(:, 1:6)', [], 1), 'int16'), 3, [])');
s2_raw = double(reshape(typecast(reshape(pkts(:, 7:12)', [], 1), 'int16'), 3, [])');

% Trim empty memory (detect 10 consecutive [-1] values)
isEmpty = all(s1_raw == -1, 2) & all(s2_raw == -1, 2);
window = 10;
if length(isEmpty) > window
    isSeq = conv(double(isEmpty), ones(window, 1), 'valid') == window;
    stopIdx = find(isSeq, 1, 'first');
    if ~isempty(stopIdx)
        valid = 1 : (stopIdx - 1);
        t_sec = t_sec(valid); s1_raw = s1_raw(valid, :); s2_raw = s2_raw(valid, :);
        fprintf('>>> Trimmed %d empty packets.\n', numPackets - length(valid));
    end
end

%% --- 4. Visualization ---
colors = [0.85 0.33 0.1; 0.47 0.67 0.19; 0 0.45 0.74];
titles = {'X-Axis / CH1', 'Y-Axis / CH2', 'Z-Axis / CH3'};

% Plot Sensor 1
figure('Name', ['Sensor 1: ' s1_Name], 'Color', 'w');
for i = 1:3
    subplot(3,1,i);
    plot(t_sec, s1_raw(:,i), 'Color', colors(i,:));
    grid on; ylabel(titles{i});
    if i == 1, title(['Raw Data: ', s1_Name]); end
end
xlabel('Time (s)');

% Plot Sensor 2
figure('Name', ['Sensor 2: ' s2_Name], 'Color', 'w');
for i = 1:3
    subplot(3,1,i);
    plot(t_sec, s2_raw(:,i), 'Color', colors(i,:));
    grid on; ylabel(titles{i});
    if i == 1, title(['Raw Data: ', s2_Name]); end
end
xlabel('Time (s)');

%% --- 5. Data Export ---
fName = sprintf('Data_%s_%s_%s', s1_Name, s2_Name, datestr(now, 'yyyy-mm-dd_HHMM'));
mkdir(fName);

% Export to MAT
save(fullfile(fName, 'dataset.mat'), 't_sec', 's1_raw', 's2_raw', 's1_Name', 's2_Name');

% Export to CSV
writetable(table(t_sec, s1_raw(:,1), s1_raw(:,2), s1_raw(:,3), ...
    'VariableNames', {'Time_sec', 'X', 'Y', 'Z'}), fullfile(fName, [s1_Name '.csv']));
writetable(table(t_sec, s2_raw(:,1), s2_raw(:,2), s2_raw(:,3), ...
    'VariableNames', {'Time_sec', 'X', 'Y', 'Z'}), fullfile(fName, [s2_Name '.csv']));

fprintf('>>> Done. Data saved to folder: %s\n', fName);
