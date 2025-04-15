clear;
close all;
t = tcpserver("127.0.0.1", 3334, "Timeout", 10);
disp("Waiting for connection...");

figure;
h = animatedline;
% axis([0 1024 -2e4 2e4]);
fs = 96000;
xlim([10,2000]);
% samples = [];
while true
    if t.NumBytesAvailable > 0
        data = read(t, t.NumBytesAvailable, "int32");
        f = linspace(-fs/2,fs/2,length(data));
        clearpoints(h);
        s = abs(fftshift(fft(data)));
        addpoints(h, f, s);
        % samples(end+1) = s;
        % addpoints(h, f, data);
        drawnow limitrate;
        sound(data - mean(data), fs);
    else
        pause(0.01);
    end
end