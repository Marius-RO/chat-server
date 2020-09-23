/*  More details at
 * https://docs.microsoft.com/en-us/windows/win32/winsock/initializing-winsock
 * */

#include <iostream>
#include <WS2tcpip.h>
#include <string>
#include <sstream>
#include <chrono>
#include <thread>

#pragma comment (lib, "ws2_32.lib")

#define SERVER_PORT "54000"
#define NEWLINE "\r\n"

using namespace std;

int main(){

    WSADATA wsaData;

    // Initialize Winsock
    int iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0) {
        cerr << "WSAStartup failed: " << iResult << NEWLINE;
        return 1;
    }

    // server part
    struct addrinfo *result = nullptr, hints{}; // hints{} means an empty initialization for hints

    ZeroMemory(&hints, sizeof (hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    // Resolve the local address and port to be used by the server
    iResult = getaddrinfo(nullptr, SERVER_PORT, &hints, &result);
    if (iResult != 0) {
        cerr << "getaddrinfo failed: " << iResult << NEWLINE;
        WSACleanup();
        return 1;
    }

    // Create a SOCKET for the server to listen for client connections
    SOCKET listeningSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (listeningSocket == INVALID_SOCKET){
        cerr << "Error when creating the socket" << NEWLINE <<  WSAGetLastError();
        WSACleanup();
        return 1;
    }


    // Setup the TCP listening socket (bind the ip address and port to a socket)
    iResult = bind(listeningSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        cerr << "bind failed with error: " << WSAGetLastError() << NEWLINE;
        freeaddrinfo(result);
        closesocket(listeningSocket);
        WSACleanup();
        return 1;
    }

    /* Once the bind function is called, the address information returned by the getaddrinfo function is no longer needed.
     * The freeaddrinfo function is called to free the memory allocated by the getaddrinfo function for this address
     * information. */
    freeaddrinfo(result);


    // Tell Winsock the socket is for listening
    if ( listen( listeningSocket, SOMAXCONN ) == SOCKET_ERROR ) {
        cerr << "Listen failed with error: " << WSAGetLastError() << NEWLINE;
        closesocket(listeningSocket);
        WSACleanup();
        return 1;
    }


    // Create the master file descriptor set and zero it
    fd_set master;
    FD_ZERO(&master);

    // Add our first socket that we're interested in interacting with; the listening socket!
    // It's important that this socket is added for our server or else we won't 'hear' incoming
    // connections
    FD_SET(listeningSocket, &master);

    cout << "---------------------------------------" << NEWLINE;
    cout << "Server is running. Waiting connections." << NEWLINE;
    cout << "---------------------------------------" << NEWLINE;

    bool running = true;
    while (running){
        // Make a copy of the master file descriptor set, this is SUPER important because
        // the call to select() is _DESTRUCTIVE_. The copy only contains the sockets that
        // are accepting inbound connection requests OR messages.

        // E.g. You have a server and it's master file descriptor set contains 5 items;
        // the listening socket and four clients. When you pass this set into select(),
        // only the sockets that are interacting with the server are returned. Let's say
        // only one client is sending a message at that time. The contents of 'copy' will
        // be one socket. You will have LOST all the other sockets.

        // SO MAKE A COPY OF THE MASTER LIST TO PASS INTO select() !!!

        fd_set copy = master;

        // See who's talking to us
        int socketCount = select(0, &copy, nullptr, nullptr, nullptr);

        // Loop through all the current connections / potential connect
        for (int i = 0; i < socketCount; i++){

            // Makes things easy for us doing this assignment
            SOCKET currentInputSocket = copy.fd_array[i];

            // Is it an inbound communication ( a new client is connected to server )
            if (currentInputSocket == listeningSocket){

                // Accept a new connection
                SOCKET clientSocket = accept(listeningSocket, nullptr, nullptr);
                if (clientSocket == INVALID_SOCKET) {
                    cerr << "accept failed: " << WSAGetLastError() << NEWLINE;
                    closesocket(listeningSocket);
                    WSACleanup();
                    return 1;
                }

                // Add the new connection to the list of connected clients
                FD_SET(clientSocket, &master);

                // Send a welcome message to the connected client
                string welcomeMessage = "---------------------------------------------------";
                welcomeMessage += NEWLINE;
                welcomeMessage += "Welcome to the Chat Server! Your socket is #" + to_string(clientSocket) + NEWLINE;
                welcomeMessage += "---------------------------------------------------";
                welcomeMessage += NEWLINE;
                send(clientSocket, welcomeMessage.c_str(), (int)welcomeMessage.size() + 1, 0);

                // show that a client is connected in server console
                cout << "Client connected on socket # " << clientSocket << NEWLINE;

            }
            else{ // It's an inbound message ( a client sent a message )

                #define BUF_LEN 4096
                char buf[BUF_LEN];
                ZeroMemory(buf, BUF_LEN);

                // Receive message
                int bytesIn = recv(currentInputSocket, buf, BUF_LEN, 0);
                if (bytesIn <= 0){
                    // Drop the client
                    closesocket(currentInputSocket);
                    FD_CLR(currentInputSocket, &master);
                }
                else{
                    // Check to see if it's a command. \quit kills the server
                    if (buf[0] == '\\'){
                        // Is the command quit?
                        string cmd = string(buf, bytesIn);
                        if (cmd == "\\quit\r\n"){
                            running = false;
                            break;
                        }

                        // Unknown command
                        continue;
                    }

                    // broadcast message to all connected clients
                    for (int j = 0; j < master.fd_count; j++){
                        SOCKET outputSocket = master.fd_array[j];
                        // don`t send message to server and to himself
                        if (outputSocket != listeningSocket && outputSocket != currentInputSocket){
                            ostringstream ss;
                            ss << "SOCKET #" << outputSocket << ": " << buf << NEWLINE;
                            string strOut = ss.str();

                            send(outputSocket, strOut.c_str(), (int)strOut.size() + 1, 0);
                        }
                    }
                }
            }
        }
    }

    //Shut down the server and close all active connections

    // Remove the listening socket from the master file descriptor set and close it
    // to prevent anyone else trying to connect.
    FD_CLR(listeningSocket, &master);
    closesocket(listeningSocket);

    // Message to let clients know what's happening.
    string message = "--------------------------------";
    message += NEWLINE;
    message += "Server is shutting down. Goodbye";
    message += NEWLINE;
    message += "--------------------------------";

    while (master.fd_count > 0){
        // Get the socket number
        SOCKET currentSocket = master.fd_array[0];

        // Send the goodbye message
        send(currentSocket, message.c_str(), (int)message.size() + 1, 0);
        // give 4 second for client to see the message
        this_thread::sleep_for(chrono::milliseconds(4000));

        // Remove it from the master file list and close the socket
        FD_CLR(currentSocket, &master);
        closesocket(currentSocket);
    }

    // Cleanup winsock
    WSACleanup();

    system("pause");

    return 0;
}