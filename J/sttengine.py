import os
import urllib.request
import zipfile
import numpy as np
import json
import queue
import threading
import sounddevice as sd
from vosk import Model, KaldiRecognizer
import socket

class VoskRealtimeSTT:
    def __init__(self):

        self.sample_rate = 22000
        self.channels = 1
        self.chunk_duration = 0.25  # seconds
        self.chunk_size = int(self.sample_rate * self.chunk_duration) #each chunk has this amount of samples

        self.audio_queue = queue.Queue() #passes audio to recognizer
        self.text_queue = queue.Queue() #passes transcriptions to be printed

        self.model_path = "vosk-model-en-us-0.22" #loading the model
        self.ensure_model()

        self.model = Model(self.model_path)
        self.recognizer = KaldiRecognizer(self.model, self.sample_rate) #create recognizer instance
        self.recognizer.SetWords(True)

        self.buffer = np.zeros((self.chunk_size, self.channels), dtype=np.float32) #buffer per chunk(audio)
        self.buffer_index = 0
        self.running = True

        self.partial_text = ""
        self.final_text = ""
        self.max_line_length = 100
        # SOCKET SETUP FOR ESP32
        self.s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.connected = False
        try:
            self.s.connect(("127.0.0.1", 1611))  # IP of your ESP32 in AP mode
            self.connected = True
            print("Connected to ESP32.")
        except Exception as e:
            print("Could not connect to ESP32, continuing without display:", e)




    def ensure_model(self): #downlaoad the model if it is needed
        if not os.path.exists(self.model_path):
            print(" Downloading Vosk model...")
            url = "https://alphacephei.com/vosk/models/vosk-model-en-us-0.22.zip"
            urllib.request.urlretrieve(url, "model.zip")
            with zipfile.ZipFile("model.zip", "r") as zip_ref:
                zip_ref.extractall(".")
            os.remove("model.zip")
            print(" Model downloaded and extracted.")

    def audio_callback(self, indata, frames, time_info, status): #used when data is ready to be used
        if status:
            print(f"Audio stream status: {status}")
#if buffer is full then reset it and restart
        remaining = min(frames, self.chunk_size - self.buffer_index)
        self.buffer[self.buffer_index:self.buffer_index + remaining] = indata[:remaining]
        self.buffer_index += remaining

        if self.buffer_index >= self.chunk_size:
            chunk = self.buffer[:self.chunk_size].flatten()
            if np.max(np.abs(chunk)) > 0:
                chunk = chunk / np.max(np.abs(chunk))  # normalize
            self.audio_queue.put(chunk.copy())
            self.buffer_index = 0

        if remaining < frames:
            leftover = frames - remaining
            self.buffer[:leftover] = indata[remaining:]
            self.buffer_index = leftover

    def process_audio(self): #stt
        while self.running:
            try:
                chunk = self.audio_queue.get(timeout=0.1)
                audio_bytes = (chunk * 32767).astype(np.int16).tobytes()

                if self.recognizer.AcceptWaveform(audio_bytes):
                    result = json.loads(self.recognizer.Result())
                    if result.get("text"):
                        new_text = result["text"]
                        if new_text:
                            self.final_text += " " + new_text
                            self.text_queue.put(self.final_text.strip())
                            self.final_text = ""
                else:
                    partial = json.loads(self.recognizer.PartialResult())
                    if partial.get("partial"):
                        current = partial["partial"].strip()
                        full = (self.final_text + " " + current).strip()
                        self.text_queue.put("\r" + full)
            except queue.Empty:
                continue
            except Exception as e:
                print("Error in processing:", e)

    def print_text(self):
        while self.running:
            try:
                text = self.text_queue.get(timeout=0.1)
                if text.startswith("\r"):
                    print(text, end="", flush=True)
                else:
                    print(text)
                    if self.connected:
                        try:
                            self.s.sendall((text + "\n").encode())  # Make sure to send as bytes
                        except Exception as e:
                            print("Error sending to ESP32:", e)
                            self.connected = False
            except queue.Empty:
                continue
            except Exception as e:
                print("Error in printing:", e)

    def start(self):
        print(" Vosk Real-Time STT Initialized. Speak now.")

        threading.Thread(target=self.process_audio, daemon=True).start()
        threading.Thread(target=self.print_text, daemon=True).start()

        try:
            with sd.InputStream(samplerate=self.sample_rate,
                                channels=self.channels,
                                dtype='float32',
                                callback=self.audio_callback,
                                blocksize=self.chunk_size):
                while self.running:
                    sd.sleep(100)
        except KeyboardInterrupt:
            print(" Interrupted by user.")
            self.running = False
        except Exception as e:
            print("Audio input error:", e)
            self.running = False

def main():
    stt = VoskRealtimeSTT()
    stt.start()

if __name__ == "__main__":
    main()