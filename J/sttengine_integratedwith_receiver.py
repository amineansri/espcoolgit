import os
import urllib.request
import zipfile
import numpy as np
import json
import queue
import threading
import socket
from vosk import Model, KaldiRecognizer

class VoskUdpSTT:
    def __init__(self):
        # STT Configuration
        self.sample_rate = 22000
        self.channels = 1
        self.model_path = "vosk-model-en-us-0.22"
        self.ensure_model()

        self.model = Model(self.model_path)
        self.recognizer = KaldiRecognizer(self.model, self.sample_rate)
        self.recognizer.SetWords(True)

        # Queues for processing
        self.audio_queue = queue.Queue()
        self.text_queue = queue.Queue()

        # Text processing variables
        self.partial_text = ""
        self.final_text = ""
        self.last_printed_text = ""  # Track last printed text to avoid duplication
        self.max_line_length = 100
        self.running = True

        # UDP configuration
        self.UDP_IP = "0.0.0.0"
        self.UDP_PORT = 3333
        self.N_SAMPLES = 120
        self.BYTES_PER_SAMPLE = 3

        # ESP32 socket setup
        self.esp_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.connected = False
        try:
            self.esp_socket.connect(("127.0.0.1", 1611))
            self.connected = True
            print("Connected to ESP32.")
        except Exception as e:
            print("Could not connect to ESP32, continuing without display:", e)

    def ensure_model(self):
        if not os.path.exists(self.model_path):
            print(" Downloading Vosk model...")
            url = "https://alphacephei.com/vosk/models/vosk-model-en-us-0.22.zip"
            urllib.request.urlretrieve(url, "model.zip")
            with zipfile.ZipFile("model.zip", "r") as zip_ref:
                zip_ref.extractall(".")
            os.remove("model.zip")
            print(" Model downloaded and extracted.")

    def decode_24bit_to_float(self, b):
        """Converts 3 bytes (little endian) to a float32 sample."""
        val = b[2] << 16 | b[1] << 8 | b[0]
        if val & 0x800000:  # Sign bit set
            val -= 0x1000000  # Convert to signed 24-bit
        return val / (2**23)

    def udp_receiver(self):
        """Receives UDP audio packets and processes them for STT"""
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.bind((self.UDP_IP, self.UDP_PORT))
        print(f"🎙️ Listening for audio on {self.UDP_IP}:{self.UDP_PORT}")

        while self.running:
            try:
                data, _ = sock.recvfrom(2048)

                # Check for valid audio packet
                if len(data) < 5 or data[0:4] != b"AUDI":
                    continue

                raw_data = data[5:]
                if len(raw_data) != 1 + (self.N_SAMPLES * self.BYTES_PER_SAMPLE):
                    continue

                mic_raw = raw_data[1:]

                # Convert 24-bit PCM to float32
                samples = np.zeros(self.N_SAMPLES, dtype=np.float32)
                for i in range(self.N_SAMPLES):
                    idx = i * 3
                    s = mic_raw[idx:idx+3]
                    samples[i] = self.decode_24bit_to_float(s)

                # Convert to PCM 16-bit for Vosk (which it expects)
                audio_bytes = (samples * 32767).astype(np.int16).tobytes()
                self.audio_queue.put(audio_bytes)

            except Exception as e:
                print(f"UDP receiver error: {e}")
                if not self.running:
                    break

    def process_audio(self):
        """Process audio chunks from the queue and run through STT"""
        while self.running:
            try:
                audio_bytes = self.audio_queue.get(timeout=0.1)

                if self.recognizer.AcceptWaveform(audio_bytes):
                    result = json.loads(self.recognizer.Result())
                    if result.get("text"):
                        new_text = result["text"]
                        if new_text:
                            # Append to accumulated text and mark it as a final result
                            self.final_text += " " + new_text
                            self.text_queue.put(("final", self.final_text.strip()))
                            self.partial_text = ""  # Clear partial buffer after final result
                else:
                    partial = json.loads(self.recognizer.PartialResult())
                    if partial.get("partial"):
                        # Store partial result but don't add to final_text yet
                        self.partial_text = partial["partial"].strip()
                        full = (self.final_text + " " + self.partial_text).strip()
                        self.text_queue.put(("partial", full))
            except queue.Empty:
                continue
            except Exception as e:
                print(f"Error in processing: {e}")

    def print_text(self):
        """Output recognized text and send to ESP32 if connected"""
        last_partial = ""  # Track the last partial text to avoid duplicates
        transcript = ""    # Complete transcript built from final results
        
        while self.running:
            try:
                result_type, text = self.text_queue.get(timeout=0.1)
                
                if result_type == "partial":
                    # Only print if different from last partial
                    if text != last_partial:
                        print(f"\r{text}", end="", flush=True)
                        last_partial = text
                        
                elif result_type == "final":
                    # Final result - update transcript and print with newline
                    if not text.strip() in transcript:  # Avoid duplicating text
                        transcript = text  # Update the complete transcript
                        print(f"\n{text}")  # Print with newline
                        
                        # Send to ESP32 if connected
                        if self.connected:
                            try:
                                self.esp_socket.sendall((text + "\n").encode())
                            except Exception as e:
                                print(f"Error sending to ESP32: {e}")
                                self.connected = False
                    
            except queue.Empty:
                continue
            except Exception as e:
                print(f"Error in printing: {e}")

    def start(self):
        """Start all processing threads"""
        print(" Vosk UDP-STT Initialized. Waiting for UDP audio...")

        # Start processing threads
        threading.Thread(target=self.udp_receiver, daemon=True).start()
        threading.Thread(target=self.process_audio, daemon=True).start()
        threading.Thread(target=self.print_text, daemon=True).start()

        # Keep main thread alive until interrupted
        try:
            while self.running:
                threading.Event().wait(1.0)
        except KeyboardInterrupt:
            print(" Interrupted by user.")
            self.running = False

def main():
    stt = VoskUdpSTT()
    stt.start()

if __name__ == "__main__":
    main()
