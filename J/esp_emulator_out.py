import socket
import time
import sys
#This script emulates the behavior of the ESP32 display
#It has the TCP socket server from the ESP as input and 
#displays the text on the console


# Set up TCP socket to listen on all interfaces, port 1611
TCP_IP = "0.0.0.0"  # Listen on all available interfaces
TCP_PORT = 1611
BUFFER_SIZE = 1024

def main():
    # Create TCP socket
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    
    try:
        server_socket.bind((TCP_IP, TCP_PORT))
        server_socket.listen(1)
        
        print(f"ESP32 Emulator running on port {TCP_PORT}")
        print(f"Listening for connections...")
        print("      Port: 1611 (same as original)")
        print("Press Ctrl+C to exit")
        
        # Accept a connection
        client_socket, client_address = server_socket.accept()
        print(f"Connection established from {client_address}")
        print("ESP32 Display Initialized")
        print("+" + "-" * 50 + "+")
        
        # Receive data
        while True:
            data = client_socket.recv(BUFFER_SIZE)
            if not data:
                print("Client disconnected")
                break
                
            text = data.decode("utf-8").strip()
            
            # Display received text
            print("\nESP32 DISPLAY:")
            print("+" + "-" * 50 + "+")
            
            # Split text into lines that fit on the display
            words = text.split()
            lines = []
            current_line = ""
            
            for word in words:
                if len(current_line + " " + word) <= 48:  # Allow some margin
                    current_line = (current_line + " " + word).strip()
                else:
                    lines.append(current_line)
                    current_line = word
                    
            if current_line:
                lines.append(current_line)
                
            # Display each line
            for line in lines:
                padding = " " * (50 - len(line))
                print(f"| {line}{padding} |")
                
            print("+" + "-" * 50 + "+")
            
    except KeyboardInterrupt:
        print("\nStopped by user")
    except Exception as e:
        print(f"Error: {e}")
    finally:
        # Close sockets
        server_socket.close()
        try:
            client_socket.close()
        except:
            pass

if __name__ == "__main__":
    main()
