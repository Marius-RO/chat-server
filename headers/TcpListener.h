#pragma once
#include "Utilities.h"
#include <WS2tcpip.h>

#pragma comment (lib, "ws2_32.lib")

class TcpListener { 

    private:
        const char*		serverIpAddress;        // IP address server will run on
        const char*		serverPort;             // port number for tcp socket
        SOCKET			serverListeningSocket;  // internal socket file descriptor for the listening socket
        fd_set			master_fd;              // master file descriptor set
        WSADATA			wsaData;                // Winsock

    protected:
        bool			serverIsRunning = true;	// server will be on until running will be set to false

        virtual void onClientConnected(SOCKET clientSocket) = 0;
        virtual void onClientDisconnected(SOCKET clientSocket) = 0;
        virtual void onMessageReceived(SOCKET clientSocket, const char* message, int length) = 0;
        virtual void sendMessageToClient(SOCKET clientSocket, const char* message, int length) = 0;
        virtual void broadcastMessageToClients(SOCKET sendingClientSocket, const char* message, int length) = 0;

    public:
        TcpListener(const char* serverIpAddress, const char* serverPort): serverIpAddress(serverIpAddress),
                    serverPort(serverPort){

            this->serverListeningSocket = INVALID_SOCKET;
            FD_ZERO(&this->master_fd);
            //TODO: initialize this->wsaData
        }

        int init(); // Initialize the listener
        int run(); // Run the listener
        virtual int shutdown(); // shut down the server

        SOCKET getServerListeningPort() { return this->serverListeningSocket; }
        fd_set getMasterFd() { return this->master_fd; }

};