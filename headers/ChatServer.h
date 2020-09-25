#pragma once
#include "TcpListener.h"

class ChatServer : public TcpListener {

    protected:
        void onClientConnected(SOCKET clientSocket) override;
        void onClientDisconnected(SOCKET clientSocket) override;
        void onMessageReceived(SOCKET clientSocket, const char* message, int length) override;
        void sendMessageToClient(SOCKET clientSocket, const char* message, int length) override;
        void broadcastMessageToClients(SOCKET sendingClientSocket, const char* message, int length) override;

    public:
        ChatServer(const char* serverIpAddress, const char* serverPort) : TcpListener(serverIpAddress, serverPort){}
        int shutdown() override;

};