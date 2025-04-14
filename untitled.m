close all;
clear;
s = readmatrix('output.csv');
fs = 32000;
% f = linspace(-fs/2,fs/2,length(s));
% plot(audio);
Ntsft = fs/100;
spectrogram(s,hann(Ntsft,"periodic"),[],Ntsft,fs,'yaxis')
% plot(f, abs(fftshift(fft(s)))/length(s));
% plot(s);
% xlabel('Sample Index');
% ylabel('Amplitude');
% yscale log;
% xscale log;
% title('Audio Waveform'); 
% xlim([0,fs/2]);