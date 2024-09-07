#ifndef SERVER_WINDOWS_HPP_
#define SERVER_WINDOWS_HPP_

#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <memory>
#include <sstream>
#include "thread_pool.hpp"
// Windows only
#ifdef _WIN32
    #include <winsock2.h>
    #pragma comment(lib, "ws2_32")
#endif

#define BUFFER_SIZE 1024

namespace server {
    struct store{
    std::string name;
    std::string password;
    int num_customer = 0;
};

bool StoreFileCreated = false;
std::unique_ptr<ThreadPool::ThreadPool> pool = ThreadPool::ThreadPool::getInstance();

SOCKET init(int port);
SOCKET CreateConnection(SOCKET server_socket);
std::shared_ptr<char[]> getRequest(SOCKET client_socket);
std::string getUrl(std::shared_ptr<char[]> buffer_ptr);


SOCKET init(int port){
    #ifdef _WIN32
        // Call WSAStartup to initialize winsock
        WSADATA wsaData;
        if(WSAStartup(MAKEWORD(2,2),&wsaData)!=0){
            std::cout<<"WSAStartUp failed"<<std::endl;
            exit(-1);
        }else std::cout<<"WSAStartUp successfully"<<std::endl;
    #endif
    

    // Create a socket
    SOCKET server_socket = socket(AF_INET,SOCK_STREAM,0);
    if(server_socket == -1){
        std::cout<<"Failed to create socket "<<GetLastError()<<std::endl;
        exit(-1);
    }else std::cout<<"Socket created successfully"<<std::endl;

    // Bind the socket to an IP address and port
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    server_address.sin_addr.S_un.S_addr = inet_addr("0.0.0.0");
    if(bind(server_socket,(struct sockaddr*)&server_address,sizeof(server_address)) == -1){
        std::cout<<"Failed to bind socket "<<GetLastError()<<std::endl;
        exit(-1);
    }else std::cout<<"Socket bound successfully"<<std::endl;

    // Start listen
    if(listen(server_socket,10)==-1){
        std::cout<<"Failed to listen "<<GetLastError()<<std::endl;
        exit(-1);
    }else{
        std::stringstream ss;
        ss<<port;
        std::cout<<"Server is listening on port "+ss.str()<<std::endl;
    }

    std::cout<<std::endl<<"Server init succesfully"<<std::endl;
    return server_socket;
}

SOCKET CreateConnection(SOCKET server_socket){
    // Awaiting for incoming connections
    struct sockaddr_in client_address;
    int client_addr_len = sizeof(struct sockaddr_in);
    SOCKET client_socket = accept(server_socket,(sockaddr*)&client_address,&client_addr_len);
    if(client_socket == -1){
        std::cout<<"Failed to accept connection "<<GetLastError()<<std::endl;
        exit(-1);
    }else std::cout<<std::endl<<"Client connected successfully"<<std::endl;
    return client_socket;
}

std::shared_ptr<char[]> getRequest(SOCKET client_socket){
    // Make request
    ThreadPool::Task task;
    ThreadPool::TaskFunction func = [](std::any params)->std::any{
        char buffer[BUFFER_SIZE] = {0};
        SOCKET server_socket = std::any_cast<SOCKET>(params);
        recv(server_socket,buffer,BUFFER_SIZE,0);
        char* buffer_ptr = buffer;
        return std::shared_ptr<char[]>(buffer_ptr,std::default_delete<char[]>());
    };
    task.result_ptr = std::promise<std::any>();
    task.name = "getRequest";
    task.params = std::any(client_socket);
    task.func = func;
    std::future<std::any> Task_future = task.result_ptr.get_future();
    pool->addTask(std::move(task));
    std::shared_ptr<char[]> buffer = std::any_cast<std::shared_ptr<char[]>>(Task_future.get());
    return std::move(buffer);
}

char* findFirst(std::shared_ptr<char[]>* buffer_ptr,int toFind){
    return strchr(buffer_ptr->get(),toFind);
}

std::string getUrl(std::shared_ptr<char[]> buffer){
    // Make url
    char* url_start = findFirst(&buffer,' ')+1;
    char* url_end = strchr(url_start,' ');
    std::string url(url_start,url_end-url_start);
    return url;
}

std::string getPath(std::string url){
    // Make path
    return "."+url;
}

void createStoreFile(){
    if(!StoreFileCreated){
        const char* delimiter = "\r\n";
        char* line = nullptr;
    }
}

}


#endif