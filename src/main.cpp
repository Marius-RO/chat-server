#include "../headers/ChatServer.h"

int main(){
	
    auto* chatServer = new ChatServer(SERVER_ADDRESS, SERVER_PORT);

    if (chatServer->init() != 0)
        return -1;

    chatServer->run();

    system("pause");

    return 0;
}