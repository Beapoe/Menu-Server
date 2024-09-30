#ifndef SERVER_WINDOWS_HPP_
#define SERVER_WINDOWS_HPP_

#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <memory>
#include <sstream>
#include "ThreadPool.hpp"
// Windows only
#ifdef _WIN32
    #include <winsock2.h>
    #pragma comment(lib, "ws2_32")
	#define GenSock SOCKET
#else
	#include <sys/socket.h>
	#include <unistd.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#define GenSock int
#endif

#define BUFFER_SIZE 1024

namespace server {
    struct store{
    std::string name;
    std::string password;
#ifdef _WIN32
    int num_customer = 0;
#else
	GenSock num_num_customer = 0;
};
#endif

#ifndef _WIN32
std::string GetLastError();
#endif
bool StoreFileCreated = false;
std::unique_ptr<ThreadPool::ThreadPool> pool = ThreadPool::ThreadPool::getInstance();

#ifndef _WIN32
std::string GetLastError(){
	return std::string(strerror(errno))+"("+std::to_string(errno)+")";
}
#endif
GenSock init(int port);
GenSock CreateConnection(GenSock server_socket);
std::unique_ptr<char[]> getRequest(GenSock client_socket);
std::string getUrl(std::shared_ptr<char[]> buffer_ptr);


GenSock init(int port){
    #ifdef _WIN32
        // Call WSAStartup to initialize winsock
        WSADATA wsaData;
        if(WSAStartup(MAKEWORD(2,2),&wsaData)!=0){
            std::cout<<"WSAStartUp failed"<<std::endl;
            exit(-1);
        }else std::cout<<"WSAStartUp successfully"<<std::endl;
    #endif
    

    // Create a socket
    GenSock server_socket = socket(AF_INET,SOCK_STREAM,0);
    if(server_socket == -1){
        std::cout<<"Failed to create socket "<<GetLastError()<<std::endl;
        exit(-1);
    }else std::cout<<"Socket created successfully"<<std::endl;

    // Bind the socket to an IP address and port
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
#ifdef _WIN32
    server_address.sin_addr.S_un.S_addr = inet_addr("0.0.0.0");
#else
	server_address.sin_addr.s_addr = inet_addr("0.0.0.0");
#endif
	socklen_t len = sizeof(server_address);
    if(bind(server_socket,(struct sockaddr*)&server_address,len) == -1){
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

GenSock CreateConnection(GenSock server_socket){
    // Awaiting for incoming connections
    struct sockaddr_in client_address;
    int client_addr_len = sizeof(struct sockaddr_in);
    GenSock client_socket = accept(server_socket,(sockaddr*)&client_address,&client_addr_len);
    if(client_socket == -1){
        std::cout<<"Failed to accept connection "<<GetLastError()<<std::endl;
        exit(-1);
    }else std::cout<<std::endl<<"Client connected successfully"<<std::endl;
    return client_socket;
}

std::unique_ptr<char[]> getRequest(GenSock client_socket){
    // Make request
    std::future<std::unique_ptr<char[]>> taskFuture = pool->addTask("getRequest",[](SOCKET client_socket)->std::unique_ptr<char[]>{
        std::unique_ptr<char[]> buffer(new char[BUFFER_SIZE]);
        int data = recv(client_socket,buffer.get(),BUFFER_SIZE,0);
        if(data == -1){
            std::cout<<"Failed to receive request "<<GetLastError()<<std::endl;
            exit(-1);
        }else std::cout<<"Request received successfully"<<std::endl;
        return std::move(buffer);
    },client_socket);
    return taskFuture.get();
}

char* findFirst(std::unique_ptr<char[]>* buffer_ptr,int toFind){
    return strchr(buffer_ptr->get(),toFind);
}

std::string getUrl(std::unique_ptr<char[]> buffer){
    // Make url
    std::future<std::string> taskFuture = pool->addTask("getUrl",[](std::unique_ptr<char[]> buffer)->std::string{
        char* url_start = findFirst(&buffer,' ')+1;
        char* url_end = strchr(url_start,' ');
        std::string url(url_start,url_end-url_start);
        return url;
    },std::move(buffer));
    return taskFuture.get();

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
