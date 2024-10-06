#ifndef SERVER_WINDOWS_HPP_
#define SERVER_WINDOWS_HPP_

#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <memory>
#include <sstream>
#include <map>
#include <fstream>
#include "ThreadPool.hpp"
// Windows only
#ifdef _WIN32
#include <winsock2.h>
#pragma comment(lib, "ws2_32")
#endif

#define BUFFER_SIZE 1024

namespace server
{
    struct Dish
    {
        std::string name;
        int price;
        std::string unit;
        unsigned long imageBinary;
    };
    struct Store
    {
        std::string name;
        std::string address;
        std::string bindPassword;
        std::string phoneNum;
        int customerAmount = 0;
        std::vector<Dish> dishes;
    };

    enum class Option
    {
        CreateStoreFile
    };

    std::map<std::string, Option> options = {
        {"/CreateStoreFile", Option::CreateStoreFile}};

    std::vector<Store> stores;
    ThreadPool::ThreadPool pool = ThreadPool::ThreadPool::getInstance();
    static const std::string base64_chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    SOCKET init(int port)
    {
#ifdef _WIN32
        // Call WSAStartup to initialize winsock
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
        {
            std::cout << "WSAStartUp failed" << std::endl;
            exit(-1);
        }
        else
            std::cout << "WSAStartUp successfully" << std::endl;
#endif

        // Create a socket
        SOCKET server_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (server_socket == -1)
        {
            std::cout << "Failed to create socket " << GetLastError() << std::endl;
            exit(-1);
        }
        else
            std::cout << "Socket created successfully" << std::endl;

        // Bind the socket to an IP address and port
        struct sockaddr_in server_address;
        server_address.sin_family = AF_INET;
        server_address.sin_port = htons(port);
        server_address.sin_addr.S_un.S_addr = inet_addr("0.0.0.0");
        if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) == -1)
        {
            std::cout << "Failed to bind socket " << GetLastError() << std::endl;
            exit(-1);
        }
        else
            std::cout << "Socket bound successfully" << std::endl;

        // Start listen
        if (listen(server_socket, 10) == -1)
        {
            std::cout << "Failed to listen " << GetLastError() << std::endl;
            exit(-1);
        }
        else
        {
            std::stringstream ss;
            ss << port;
            std::cout << "Server is listening on port " + ss.str() << std::endl;
        }

        std::cout << std::endl
                  << "Server init succesfully" << std::endl;
        return server_socket;
    }

    SOCKET CreateConnection(SOCKET server_socket)
    {
        // Awaiting for incoming connections
        struct sockaddr_in client_address;
        int client_addr_len = sizeof(struct sockaddr_in);
        SOCKET client_socket = accept(server_socket, (sockaddr *)&client_address, &client_addr_len);
        if (client_socket == -1)
        {
            std::cout << "Failed to accept connection " << GetLastError() << std::endl;
            exit(-1);
        }
        else
            std::cout << std::endl
                      << "Client connected successfully" << std::endl;
        return client_socket;
    }

    std::shared_ptr<char[]> getRequest(SOCKET client_socket)
    {
        // Make request
        std::future<std::shared_ptr<char[]>> taskFuture = pool.addTask("getRequest", [](SOCKET client_socket)
                                                                       {
            std::shared_ptr<char[]> buffer(new char[BUFFER_SIZE]);
            int data = recv(client_socket, buffer.get(), BUFFER_SIZE, 0);
            if (data == -1)
            {
                std::cout << "Failed to receive request " << GetLastError() << std::endl;
                exit(-1);
            }
            else
                std::cout << "Request received successfully" << std::endl;
            return std::move(buffer); }, client_socket);
        return taskFuture.get();
    }

    char *findFirst(std::shared_ptr<char[]> *buffer_ptr, int toFind)
    {
        return strchr(buffer_ptr->get(), toFind);
    }

    std::string getUrl(std::shared_ptr<char[]> buffer)
    {
        // Make url
        std::future<std::string> taskFuture = pool.addTask("getUrl", [](std::shared_ptr<char[]> buffer)
                                                           {
            char *url_start = findFirst(&buffer, ' ') + 1;
            char *url_end = strchr(url_start, '?');
            std::string url(url_start, url_end - url_start);
            return url; }, std::move(buffer));
        return taskFuture.get();
    }

    std::string getPath(std::string url)
    {
        // Make path
        return "." + url;
    }

    Option getOption(std::string url)
    {
        auto it = options.find(url);
        if (it != options.end())
        {
            return it->second;
        }
        else
        {
            throw std::runtime_error("Option not found");
        }
    }

    std::string getParameterPair(std::string &str, char startChar, char endChar)
    {
        size_t startPos = 0;
        if (startChar != ' ')
        {
            size_t startPos = str.find(startChar);
            if (startPos == std::string::npos)
            {
                return ""; // 找不到起始字符
            }
        }

        size_t endPos = str.find(endChar, startPos + 1);
        if (endPos == std::string::npos)
        {
            return ""; // 找不到结束字符
        }

        std::string result = str.substr(startPos, endPos - startPos - 1);
        str.erase(startPos, endPos - startPos + 1);
        return result;
    }

    std::vector<std::vector<std::string>> getParameters(std::string request)
    {
        size_t position = request.find("?");
        size_t endPosition = request.find(" ", position);
        if (position != std::string::npos)
        {
            std::string paras = request.substr(position + 1, endPosition - position - 1) + "&";
            std::vector<std::string> parametersPairs;
            while (!paras.empty())
                parametersPairs.push_back(getParameterPair(paras, ' ', '&'));
            std::vector<std::vector<std::string>> parameters;
            std::vector<std::string> keyValue;
            for (auto parameterPair : parametersPairs)
            {
                size_t pos = parameterPair.find("=");
                keyValue.push_back(parameterPair.substr(0, pos));
                keyValue.push_back(parameterPair.substr(pos + 1, parameterPair.length() - pos - 1));
                parameters.push_back(keyValue);
                keyValue.clear();
            }
            return parameters;
        }
        else
        {
            throw std::runtime_error("Parameters not found");
        }
    }

    bool isBase64(const std::string &str)
    {
        for (std::size_t i = 0; i < str.size(); ++i)
        {
            if (base64_chars.find(str[i]) == std::string::npos)
            {
                return false;
            }
        }
        return true;
    }

    std::string base64_decode(const std::string &encoded_string)
    {
        int in_len = encoded_string.size();
        int i = 0;
        int j = 0;
        int in_ = 0;
        unsigned char char_array_4[4], char_array_3[3];

        std::string decoded_string;

        while (in_len-- && (encoded_string[in_] != '=') && isBase64(std::string(1, encoded_string[in_])))
        {
            char_array_4[i++] = encoded_string[in_];
            if (i == 4)
            {
                for (i = 0; i < 4; i++)
                    char_array_4[i] = base64_chars.find(char_array_4[i]);

                char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
                char_array_3[1] = ((char_array_4[1] & 0x0f) << 4) + ((char_array_4[2] & 0x3c) >> 2);
                char_array_3[2] = ((char_array_4[2] & 0x03) << 6) + char_array_4[3];

                decoded_string += (char_array_3[0]);
                if (--j)
                    decoded_string += (char_array_3[1]);
                if (--j)
                    decoded_string += (char_array_3[2]);

                i = 0;
            }
            in_++;
        }

        if (i)
        {
            for (j = i; j < 4; j++)
                char_array_4[j] = 0;

            for (j = 0; j < 4; j++)
                char_array_4[j] = base64_chars.find(char_array_4[j]);

            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0x0f) << 4) + ((char_array_4[2] & 0x3c) >> 2);

            decoded_string += (char_array_3[0]);
            if (--j)
                decoded_string += (char_array_3[1]);
        }

        return decoded_string;
    }

    std::wstring s2ws(const std::string &s)
    {
        int len = MultiByteToWideChar(CP_ACP, 0, s.c_str(), -1, nullptr, 0);
        std::wstring wstr(len, 0);
        MultiByteToWideChar(CP_ACP, 0, s.c_str(), -1, &wstr[0], len);
        return wstr;
    }

    std::string ws2a(std::wstring &wstr)
    {
        int len = WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
        std::string ansi(len, 0);
        WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, &ansi[0], len, nullptr, nullptr);
        return ansi;
    }

    void write2WindowsTXT(std::string str, std::ofstream &file)
    {
        std::wstring wstr = s2ws(str);
        std::string ansi = ws2a(wstr);
        file.write(ansi.c_str(), ansi.length());
    }

    void createStoreFile(std::string url)
    {
        std::vector<std::vector<std::string>> parameters = getParameters(url);
        std::ofstream storesList("storesList.txt", std::ios::app | std::ios::binary);
        if (!storesList)
        {
            throw std::runtime_error("Failed to open storesList.txt");
        }
        else
        {
            std::vector<std::string> params = {"name", "address", "bindPassword", "phoneNum"};
            storesList << "{" << std::endl;
            for (int i = 0; i < parameters.size(); i++)
            {
                if (parameters[i][0] == params[i])
                {
                    if (isBase64(parameters[i][1]))
                    {
                        parameters[i][1] = base64_decode(parameters[i][1]);
                    }
                }
                else
                {
                    throw std::runtime_error("Parameters not found");
                }
            }
            for (int i = 0; i < parameters.size(); i++)
            {
                write2WindowsTXT(parameters[i][1] + ":", storesList);
                write2WindowsTXT(parameters[i][2], storesList);//base64判断方法和解码方法有问题
                storesList << std::endl;
            }
            storesList << "}" << std::endl;
        }
        storesList.close();
    }

    void Execute(SOCKET client_socket)
    {
        std::shared_ptr<char[]> request = getRequest(client_socket);
        std::string url = getUrl(request);
        std::cout << url << std::endl;
        Option option = getOption(url);
        switch (option)
        {
        case Option::CreateStoreFile:
            createStoreFile(request.get());
            break;
        default:
            throw std::runtime_error("Option not found");
            break;
        }
    }
}

#endif