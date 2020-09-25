#include "../headers/ChatServer.h"
#include <chrono>
#include <thread>

void ChatServer::onClientConnected(SOCKET clientSocket) {

    // Send a welcome message to the connected client
    string welcomeMessage = NEWLINE;
    welcomeMessage += "---------------------------------------------------";
    welcomeMessage += NEWLINE;
    welcomeMessage += "Welcome to the Chat Server! Your socket is #" + to_string(clientSocket) + NEWLINE;
    welcomeMessage += "---------------------------------------------------";
    welcomeMessage += NEWLINE;

    sendMessageToClient(clientSocket, welcomeMessage.c_str(), (int)welcomeMessage.size() + 1);

    // show that a client is connected in server console
    cout << "Client connected on socket # " << clientSocket << NEWLINE;
				
}

void ChatServer::onClientDisconnected(SOCKET clientSocket) {
    cout << "Client #" << clientSocket << " disconnected" << NEWLINE;
}

void ChatServer::onMessageReceived(SOCKET clientSocket, const char* message, int length) {

    // Check to see if it's a command. \quit kills the server
    if (message[0] == '\\') {
        // Is the command quit?
        string cmd = string(message, length);
        if (cmd == "\\quit\r\n") {
            this->serverIsRunning = false;
            return;
        }
    }

    broadcastMessageToClients(clientSocket, message, length);
}

void ChatServer::broadcastMessageToClients(SOCKET sendingClientSocket, const char* message, int length) {

    SOCKET serverListeningSocket = this->getServerListeningPort();
    fd_set master_fd = this->getMasterFd();

    // broadcast message to all connected clients
    for (unsigned int i = 0; i < master_fd.fd_count; i++) {
        SOCKET outputSocket = master_fd.fd_array[i];
        // don`t send message to server and to himself
        if (outputSocket != serverListeningSocket && outputSocket != sendingClientSocket) {
            sendMessageToClient(outputSocket, message, length);
        }
    }
}

void ChatServer::sendMessageToClient(SOCKET clientSocket, const char* message, int length) {

    ostringstream ss;
    ss << "SOCKET #" << clientSocket << ": " << message << NEWLINE;
    string strOut = ss.str();

    send(clientSocket, strOut.c_str(), (int)strOut.size() + 1, 0);
}

int ChatServer::shutdown() {

    //Shut down the server and close all active connections

    // Remove the listening socket from the master file descriptor set and close it
    // to prevent anyone else trying to connect.
    SOCKET serverListeningSocket = this->getServerListeningPort();
    fd_set master_fd = this->getMasterFd();

    FD_CLR(serverListeningSocket, &master_fd);
    closesocket(serverListeningSocket);

    // Message to let clients know what's happening.
    string message = "--------------------------------";
    message += NEWLINE;
    message += "Server is shutting down. Goodbye";
    message += NEWLINE;
    message += "--------------------------------";

    while (master_fd.fd_count > 0) {
        // Get the socket number
        SOCKET currentSocket = master_fd.fd_array[0];

        // Send the goodbye message
        send(currentSocket, message.c_str(), (int)message.size() + 1, 0);
        // give 4 second for client to see the message
        this_thread::sleep_for(chrono::milliseconds(4000));

        // Remove it from the master file list and close the socket
        FD_CLR(currentSocket, &master_fd);
        closesocket(currentSocket);
    }

    // Cleanup winsock
    WSACleanup();

    return 0;

}