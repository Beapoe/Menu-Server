#include <iostream>
#include "server.hpp"

void test(){
    SOCKET server_socket = init(1024);
    CreateConnection(server_socket);
    std::cout<<*getRequest(server_socket)<<std::endl;
}

int main(){
    
    return 0;
}