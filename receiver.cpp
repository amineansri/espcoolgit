#include <arpa/inet.h> // For sockaddr_in and inet_ntoa()
#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sys/socket.h> // For socket functions
#include <unistd.h>     // For close()
#include <fstream>
#include <ctime>

#include "BeamFormer.h"
// #include "FrostBeamformer.hpp"

#define MIC_AMOUNT 4
#define RAW_MIC_LENGTH 360
#define MIC_LENGTH 120

int sockfd, sockfd1; // Socket file descriptor
std::ofstream file("output.csv");

void handle_sigint(int sig) {
  // Close the socket and exit
  close(sockfd);
  close(sockfd1);
  file.close();
  std::cout << "\nServer terminated gracefully" << std::endl;
  exit(0);
}

std::vector<std::vector<int32_t>> process_mic(char* data, size_t n) {
    if (!(data[0] == 'A' && data[1] == 'U' && data[2] == 'D' && data[3] == 'I')){
        return {};
    }
    char seq = data[4];

    char* raw_data = (char*) malloc(n - 5);
    memcpy(raw_data, &data[5],n-5);
    

    char** raw_mic_data = (char**) malloc(MIC_AMOUNT * sizeof(char*));
    for (int i = 0; i < MIC_AMOUNT; i++) {
        raw_mic_data[i] = (char*) malloc(RAW_MIC_LENGTH);
    }

    uint16_t offset = 0;
    for(int i = 0; i < MIC_AMOUNT; i++) {
        memcpy(raw_mic_data[i], &raw_data[++offset], RAW_MIC_LENGTH);
        offset += RAW_MIC_LENGTH;
    }
    free(raw_data);

    // int32_t** mic_data = (int32_t**) malloc(MIC_AMOUNT * sizeof(int32_t*));
    // for (int i = 0; i < MIC_AMOUNT; i++) {
    //     mic_data[i] = (int32_t*) malloc(MIC_LENGTH * sizeof(int32_t*));
    // }

    std::vector<std::vector<int32_t>> mic_data(MIC_AMOUNT, std::vector<int32_t>(MIC_LENGTH));

    for(int j = 0; j < MIC_AMOUNT; j++) {
        char* rmd = raw_mic_data[j];
        for(int i = 0; i < RAW_MIC_LENGTH; i += 3) {
            mic_data[j][i/3] = (rmd[i+2] << 16 |  rmd[i+1] << 8 | rmd[i]) | (rmd[i+2] >> 7 == 0x01 ? 0xFF000000 : 0x00000000);
        }
    }

    
    for(int i = 0; i < MIC_AMOUNT; i++) {
        free(raw_mic_data[i]);
    }
    free(raw_mic_data);


    // for(int j = 0; j < MIC_AMOUNT; j++) {
    //     std::cout << j << ": [";
    //     for(int i = 0; i < MIC_LENGTH-1; i++) {
    //         std::cout << mic_data[j][i] << ", ";
    //     }
    //     std::cout << mic_data[j][MIC_LENGTH-1] << "]" << std::endl;
    // }
    

    // for(int i = 0; i < MIC_AMOUNT; i++) {
    //     free(mic_data[i]);
    // }
    // free(mic_data);
    return mic_data;
}

void send_audio_samples(const std::vector<int32_t>& samples, int sockfd) {
    size_t num_bytes = samples.size() * sizeof(int32_t);
    send(sockfd, samples.data(), num_bytes, 0);
}

int main() {
  struct sockaddr_in server_addr, client_addr;
  char buffer[2048];
  socklen_t addr_len = sizeof(client_addr);
  
  Beamformer beamer = Beamformer(32000, 0.0);
  // FrostBeamformer beamer;

  // Set up the signal handler for Ctrl+C (SIGINT)
  signal(SIGINT, handle_sigint);

  // Create a UDP socket
  if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    perror("Socket creation failed");
    exit(EXIT_FAILURE);
  }

  // Zero out the server address structure
  memset(&server_addr, 0, sizeof(server_addr));

  // Set up the server address
  server_addr.sin_family = AF_INET;         // IPv4
  server_addr.sin_addr.s_addr = INADDR_ANY; // Bind to all interfaces
  server_addr.sin_port = htons(3333);       // Port 8081

  // Bind the socket to the server address
  if (bind(sockfd, (const struct sockaddr *)&server_addr, sizeof(server_addr)) <
      0) {
    perror("Bind failed");
    close(sockfd);
    exit(EXIT_FAILURE);
  }

  std::cout << "C++ UDP server listening on port 3333" << std::endl;

  int sockfd1 = socket(AF_INET, SOCK_STREAM, 0);
  sockaddr_in serv_addr{};
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(3334);
  inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);

  if (connect(sockfd1, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
      std::cerr << "Connection failed\n";
      return -1;
  }


  std::vector<int32_t> output;
  std::time_t start = std::time(nullptr);
  std::vector<int32_t> tosend;
  while (true) {
    // Receive a message from a client
    ssize_t n = recvfrom(sockfd, buffer, sizeof(buffer) - 1, 0,
                         (struct sockaddr *)&client_addr, &addr_len);
    if (n < 0) {
      perror("Receive failed");
      continue;
    }

    buffer[n] = '\0'; // Null-terminate the received data

    // Print the received message
    output = beamer.run(process_mic(buffer, n));
    if (tosend.size() > 2000) {
      send_audio_samples(tosend, sockfd1);
      tosend.clear();
    } else {
      tosend.insert( tosend.end(), output.begin(), output.end() );
    }
    // output = beamformer.filter(process_mic(buffer, n));
    
    // for (const auto& sample : output) {
    //     file << sample << "\n";
    // }
    // if (std::time(nullptr) - start >= 5) {
    //   handle_sigint(0);
    //   return 0;
    // }
    // return 0;

    // std::cout << "C++ server received: " << buffer << std::endl;
  }

  // Close the socket (unreachable code in this example)
  close(sockfd);

  return 0;
}