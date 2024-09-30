#include "Server.hpp"

void test(){
    SOCKET server_socket = server::init(1024);
    SOCKET client_socket = server::CreateConnection(server_socket);
    std::cout<<server::getUrl(server::getRequest(client_socket))<<std::endl;// 未知原因导致url打印异常
}

int main(){
    test();
    return 0;
}
