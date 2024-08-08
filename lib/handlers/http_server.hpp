#pragma once

#ifndef HTTP_SERVER_HPP
#define HTTP_SERVER_HPP

#include <string>
#include <vector>
#include <stdio.h>
#include <sstream>
#include <fstream>
#include <stdlib.h>
#include <iostream>

#ifdef _WIN32
  #include <winsock.h>
#else
  #include <cstring>
  #include <unistd.h>
  #include <arpa/inet.h>
  #include <sys/socket.h>
#endif

#include "../helpers/debug_helper.hpp"
#include "../helpers/parse_helper.hpp"
#include "../structs/server_config_struct.hpp"
#include "../enums/validation_flag_result_enum.hpp"

struct RequestHeader {
  std::string method;
  std::string endpoint;
  std::string content;
  std::string accept;
  std::string content_type;
  std::string content_length;

  // Make deconstructor
  ~RequestHeader() {
    method = "";
    endpoint = "";
    content = "";
    accept = "";
    content_type = "";
    content_length = "";
  }
};

namespace http {
  class TCPServer {
    public:
      /**
       * @brief Default constructor for TCPServer
       * 
       * @param hostname (std::string) - The hostname for the server
       * @param port (int) - The port for the server
       * 
       * @return void
       */
      TCPServer(std::string hostname, int port) :
        server_hostname(hostname), server_port(port),
        server_socket(), server_new_socket(),
        server_socket_len(sizeof(server_address)),
        server_response_message(buildDefaultResponseMessage())
      {
        handleConstructServer();
      }

      /**
       * @brief Constructor for TCPServer with ServerConfig
       * 
       * @param hostname (std::string) - The hostname for the server
       * @param port (int) - The port for the server
       * 
       * @return void
       */
      TCPServer(ServerConfig config) :
        server_hostname(config.hostname), server_port(config.port),
        server_socket(), server_new_socket(),
        server_socket_len(sizeof(server_address)),
        server_response_message(buildDefaultResponseMessage())
      {
        handleConstructServer();
      }

      /**
       * @brief Default destructor for TCPServer
       * 
       * @return void
       */
      ~TCPServer() {
        closeServer();
      }

      /**
       * @brief Run the server
       * 
       * @return void
       */
      void run() {
        startListen();
      }

    private:
      // Global variables
      int server_port;
      long server_new_request;
      std::string server_hostname;
      std::string server_response_message;

      // Define socket address struct
      struct sockaddr_in server_address;

      // Define variables for win32 and unix
      #ifdef _WIN32
        int server_socket_len;
        SOCKET server_socket;
        SOCKET server_new_socket;
        WSADATA server_wsa_data;
      #else
        int server_socket;
        int server_new_socket;
        unsigned int server_socket_len;
      #endif

      // Start Defining Methods
      void handleConstructServer() {
        #ifdef _WIN32
          // Construct wsa data
          server_wsa_data = WSADATA();
        #endif

        server_address.sin_family = AF_INET;
        server_address.sin_port = htons(server_port);
        server_address.sin_addr.s_addr = inet_addr(server_hostname.c_str());

        // Check if port already used on local machine
        if (server_socket = socket(AF_INET, SOCK_STREAM, 0) < 0) {
          debug::quit(parse::parseMessage("ERROR: Port %1s already in use.\n\nPlease configure a different port in the server configuration.", {std::to_string(server_socket)}));
        }

        // Double check if socket is created
        if (startServer() != 0) {
          debug::quit(parse::parseMessage("ERROR: Could not create socket on port %1s.\n\nPlease configure a different port in the server configuration.", {std::to_string(server_port)}));
        }
      }

      void closeServer() {
        #ifdef _WIN32
          closesocket(server_socket);
          closesocket(server_new_socket);
          WSACleanup();
        #else
          close(server_socket);
          close(server_new_socket);
        #endif

        exit(0);
      }

      std::string buildDefaultResponseMessage(const std::string path = "./view/home.html") {
        // Read the file from './view/home.html'
        std::ifstream file(path);
        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

        // Build the response message
        std::string response_message = "HTTP/1.1 200 OK\nContent-Type: text/html\nContent-Length: " + std::to_string(content.size()) + "\n\n" + content;

        return response_message;
      }

      int startServer() {
        #ifdef _WIN32
          // Initialize Winsock
          if (WSAStartup(MAKEWORD(2, 2), &server_wsa_data) != 0) {
            debug::quit("Failed to initialize Winsock.");
          }
        #endif

        server_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (server_socket < 0) {
          debug::quit("Failed to create socket.");
        }

        if (bind(server_socket, (sockaddr *)&server_address, server_socket_len) < 0) {
          debug::quit("Failed to bind socket.");
        }

        return 0;
      }

      void acceptConnection() {
        server_new_socket = accept(server_socket, (sockaddr *)&server_address, &server_socket_len);

        if (server_new_socket < 0) {
          debug::quit(parse::parseMessage("ERROR: Failed to accept connection from address %1s:%2s", {inet_ntoa(server_address.sin_addr), std::to_string(ntohs(server_address.sin_port))}));
        } else {
          debug::print(parse::parseMessage("INFO: Accepted connection from %1s:%2s", {inet_ntoa(server_address.sin_addr), std::to_string(ntohs(server_address.sin_port))}));
        }
      }

      void sendResponse() {
        long totalBytesSent = 0;

        #ifdef _WIN32
          int bytesSent;

          while (totalBytesSent < server_response_message.size()) {
            bytesSent = send(server_new_socket, server_response_message.c_str(), server_response_message.size(), 0);
            if (bytesSent < 0) {
              break;
            }

            totalBytesSent += bytesSent;
          }

          if (bytesSent < 0 || totalBytesSent != server_response_message.size()) debug::print("Failed to send response to client.");
        #else
          totalBytesSent = write(server_new_socket, server_response_message.c_str(), server_response_message.size());

          if (totalBytesSent < 0 || totalBytesSent != server_response_message.size()) debug::print("Failed to send response to client.");
        #endif
      }

      void sendDefaultResponse(const RequestHeader &header) {
        // Check if accept wants json, with regex */json
        if (header.accept.find("/json") != std::string::npos) {
          std::string json = "{ \"status\": \"Server is running.\", \"message\": \"Selamat datang di Merdeka CTF.\" }";
          server_response_message = "HTTP/1.1 200 OK\nContent-Type: application/json\nContent-Length: " + std::to_string(json.size()) + "\n\n" + json;
        } else {
          server_response_message = buildDefaultResponseMessage();
        }

        sendResponse();
      }

      void sendErrorResponse(const RequestHeader &header) {
        // Check if accept wants json, with regex */json
        if (header.accept.find("/json") != std::string::npos) {
          std::string json = "{ \"status\": \"Server is running.\", \"message\": \"Terjadi error pada saat melakukan proses handle request.\" }";
          server_response_message = "HTTP/1.1 200 OK\nContent-Type: application/json\nContent-Length: " + std::to_string(json.size()) + "\n\n" + json;
        } else {
          server_response_message = buildDefaultResponseMessage("./view/error.html");
        }

        sendResponse();
      }

      void sendFlagResponse(const RequestHeader &header) {
        // Check if accept wants json, with regex */json
        if (header.accept.find("/json") != std::string::npos) {
          std::string json = "{ \"status\": \"Server is running.\", \"message\": \"Selamat kamu berhasil dapetin flag nya\", \"flag\": \"MerdekaHack{4Thur_S4y4Ng_k4L14N_s3Mu4_k0K}\" }";
          server_response_message = "HTTP/1.1 200 OK\nContent-Type: application/json\nContent-Length: " + std::to_string(json.size()) + "\n\n" + json;
        } else {
          server_response_message = buildDefaultResponseMessage();
        }

        sendResponse();
      }

      ValidationFlagResult validateRequestForFlag(const RequestHeader &header) {
        if (!(header.endpoint.find("/flag") != std::string::npos)) return ValidationFlagResult::VALIDATION_FLAG_ERROR;
        if (!(header.method.find("PATCH") != std::string::npos)) return ValidationFlagResult::VALIDATION_FLAG_ERROR;
        if (!(header.accept.find("/json") != std::string::npos)) return ValidationFlagResult::VALIDATION_FLAG_WARNING;
        if (!(header.content_type.find("application/json") != std::string::npos)) return ValidationFlagResult::VALIDATION_FLAG_WARNING;

        // Get the content
        std::string content = header.content;

        if (!(content.find("role") != std::string::npos)) return ValidationFlagResult::VALIDATION_FLAG_WARNING;
        if (!(content.find("admin") != std::string::npos)) return ValidationFlagResult::VALIDATION_FLAG_WARNING;
      
        return ValidationFlagResult::VALIDATION_FLAG_SUCCESS;
      }

      /**
       * @brief Get the Request Header object
       * 
       * @param buffer (char[]) - The buffer to parse
       * 
       * @return RequestHeader 
       */
      RequestHeader getRequestHeader(char buffer[]) {
        /**
         * Buffer will contain structure like this:
         * POST /login HTTP/1.1
         * content-length: 22
         * accept-encoding: gzip, deflate, br
         * Accept: *\/\*
         * User-Agent: Thunder Client (https://www.thunderclient.com)
         * Content-Type: application/json
         * Host: 127.0.0.1:8080
         * Connection: close
         *  
         * {
         *   "username": "awikwok"
         * }
         */
        RequestHeader header;

        // Make string stream from buffer
        std::istringstream ss(buffer);
        std::string line;

        // Read the first line to get method and endpoint
        std::getline(ss, line);
        std::istringstream line_ss(line);
        line_ss >> header.method >> header.endpoint;

        // Read the rest of the lines to get content, accept, content-type, and content-length
        while (std::getline(ss, line) && !line.empty()) {
          if (line.find("Content-Length:") != std::string::npos || line.find("content-length:") != std::string::npos) {
            header.content_length = line.substr(line.find(":") + 2, line.length());
          } else if (line.find("Accept:") != std::string::npos || line.find("accept:") != std::string::npos) {
            header.accept = line.substr(line.find(":") + 2, line.length());
          } else if (line.find("Content-Type:") != std::string::npos || line.find("content-type:") != std::string::npos) {
            header.content_type = line.substr(line.find(":") + 2, line.length() - 1);
          }
        }

        if (
          parse::parseToLower(header.method) == "get" ||
          parse::parseToLower(header.method) == "head" ||
          parse::parseToLower(header.method) == "options"
        ) return header;

        if (header.content_length.empty()) return header;
        if (std::stoi(header.content_length) <= 0) return header;

        // Get the content
        std::string content = ss.str();
        content = content.substr(content.find("{"), std::stoi(header.content_length));
        header.content = content;

        return header;
      }

      void startListen() {
        if (listen(server_socket, 20) < 0) {
          debug::quit("Failed to listen on socket.");
        }

        debug::print(parse::parseMessage("Server is running, Listening on address: %1s:%2s", {server_hostname, std::to_string(server_port)}));

        int bytesReceived;

        while (true) {
          acceptConnection();

          char buffer[128000] = {0};
          bytesReceived = recv(server_new_socket, buffer, 30720, 0);

          if (bytesReceived < 0) {
            debug::quit("Failed to receive data from client.");
          }

          RequestHeader header = getRequestHeader(buffer);

          if (header.method.empty()) {
            debug::quit("Failed to parse request header.");
          }

          ValidationFlagResult flagResult = validateRequestForFlag(header);

          switch (flagResult) {
          case ValidationFlagResult::VALIDATION_FLAG_ERROR:
            sendDefaultResponse(header);
            break;
          case ValidationFlagResult::VALIDATION_FLAG_WARNING:
            sendErrorResponse(header);
            break;
          case ValidationFlagResult::VALIDATION_FLAG_SUCCESS:
            sendFlagResponse(header);
            break;
          default:
            sendDefaultResponse(header);
            break;
          }

          #ifdef _WIN32
            closesocket(server_new_socket);
          #else
            close(server_new_socket);
          #endif

          // Reset buffer
          memset(buffer, 0, sizeof(buffer));

          debug::print("INFO: Connection closed.");
        }
      }
  };
}

#endif // HTTP_SERVER_HPP
