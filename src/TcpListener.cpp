#include "../headers/TcpListener.h"

int TcpListener::init() {

    // Initialize Winsock
    int iResult = WSAStartup(MAKEWORD(2, 2), &this->wsaData);
    if (iResult != 0) {
        cerr << "WSAStartup failed: " << iResult << NEWLINE;
        return 1;
    }

    // server part
    struct addrinfo* result = nullptr, hints{}; // hints{} means an empty initialization for hints

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    // Resolve the local address and port to be used by the server
    iResult = getaddrinfo(nullptr, this->serverPort, &hints, &result);
    if (iResult != 0) {
        cerr << "getaddrinfo failed: " << iResult << NEWLINE;
        WSACleanup();
        return 1;
    }


    // Create a SOCKET for the server to listen for client connections
    this->serverListeningSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (this->serverListeningSocket == INVALID_SOCKET) {
        cerr << "Error when creating the socket" << NEWLINE << WSAGetLastError();
        WSACleanup();
        return 1;
    }

    // Setup the TCP listening socket (bind the ip address and port to a socket)
    iResult = bind(this->serverListeningSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        cerr << "bind failed with error: " << WSAGetLastError() << NEWLINE;
        freeaddrinfo(result);
        closesocket(this->serverListeningSocket);
        WSACleanup();
        return 1;
    }


    /* Once the bind function is called, the address information returned by the getaddrinfo function is no longer needed.
    * The freeaddrinfo function is called to free the memory allocated by the getaddrinfo function for this address
    * information. */
    freeaddrinfo(result);


    // Tell Winsock the socket is for listening
    if (listen(this->serverListeningSocket, SOMAXCONN) == SOCKET_ERROR) {
        cerr << "Listen failed with error: " << WSAGetLastError() << NEWLINE;
        closesocket(this->serverListeningSocket);
        WSACleanup();
        return 1;
    }

    // Set zero to master file descriptor (done in constructor)
    //FD_ZERO(&this->master_fd);

    // Add our first socket that we're interested in interacting with; the listening socket!
    // It's important that this socket is added for our server or else we won't 'hear' incoming
    // connections
    FD_SET(this->serverListeningSocket, &this->master_fd);

    cout << "---------------------------------------" << NEWLINE;
    cout << "Server is running. Waiting connections." << NEWLINE;
    cout << "---------------------------------------" << NEWLINE;

    return 0;
}

int TcpListener::run() {

    while (this->serverIsRunning) {
        // Make a copy of the master file descriptor set, this is SUPER important because
        // the call to select() is _DESTRUCTIVE_. The copy only contains the sockets that
        // are accepting inbound connection requests OR messages.

        // E.g. You have a server and it's master file descriptor set contains 5 items;
        // the listening socket and four clients. When you pass this set into select(),
        // only the sockets that are interacting with the server are returned. Let's say
        // only one client is sending a message at that time. The contents of 'copy' will
        // be one socket. You will have LOST all the other sockets.

        // SO MAKE A COPY OF THE MASTER LIST TO PASS INTO select() !!!

        fd_set copy = this->master_fd;

        // See who's talking to us
        int socketCount = select(0, &copy, nullptr, nullptr, nullptr);

        // Loop through all the current connections / potential connect
        for (int i = 0; i < socketCount; i++) {

            // Makes things easy for us doing this assignment
            SOCKET currentInputSocket = copy.fd_array[i];

            // Is it an inbound communication ( a new client is connected to server )
            if (currentInputSocket == this->serverListeningSocket) {

                // Accept a new connection
                SOCKET clientSocket = accept(this->serverListeningSocket, nullptr, nullptr);
                if (clientSocket == INVALID_SOCKET) {
                    cerr << "Accept failed: " << WSAGetLastError() << NEWLINE;
                    closesocket(this->serverListeningSocket);
                    WSACleanup();
                    return 1;
                }

                // Add the new connection to the list of connected clients
                FD_SET(clientSocket, &this->master_fd);

                onClientConnected(clientSocket);
            }
            else { // It's an inbound message ( a client sent a message )

                #define BUF_LEN 4096
                char buf[BUF_LEN];
                ZeroMemory(buf, BUF_LEN);

                // Receive message
                int bytesIn = recv(currentInputSocket, buf, BUF_LEN, 0);
                if (bytesIn <= 0) {
                    // Drop the client
                    onClientDisconnected(currentInputSocket);
                    closesocket(currentInputSocket);
                    FD_CLR(currentInputSocket, &this->master_fd);
                }
                else {

                    onMessageReceived(currentInputSocket, buf, bytesIn);
                }
            }
        }
    }

    //Shut down the server and close all active connections
    return this->shutdown();

}

int TcpListener::shutdown() {

    // Remove the listening socket from the master file descriptor set and close it
    // to prevent anyone else trying to connect.
    FD_CLR(this->serverListeningSocket, &this->master_fd);
    closesocket(this->serverListeningSocket);

    while (this->master_fd.fd_count > 0) {
        // Get the socket number
        SOCKET currentSocket = this->master_fd.fd_array[0];

        // Remove it from the master file list and close the socket
        FD_CLR(currentSocket, &this->master_fd);
        closesocket(currentSocket);
    }

    WSACleanup();
	
    return 0;

}

