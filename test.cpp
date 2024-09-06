#include <iostream>
#include "server.hpp"

void test(){
    SOCKET server_socket = server::init(1024);
    SOCKET clien_socket = server::CreateConnection(server_socket);
    std::cout<<server::getUrl(server::getRequest(clien_socket))<<std::endl;
}

int main(){
    
    return 0;
}