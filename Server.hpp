#ifndef SERVER_WINDOWS_HPP_
#define SERVER_WINDOWS_HPP_

#include <iostream>
#include <algorithm>
#include <stdexcept>
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

#if __cplusplus >= 201703L
#include <string_view>
#endif // __cplusplus >= 201703L

    std::string base64_encode(std::string const &s, bool url = false);
    std::string base64_encode_pem(std::string const &s);
    std::string base64_encode_mime(std::string const &s);

    std::string base64_decode(std::string const &s, bool remove_linebreaks = false);
    std::string base64_encode(unsigned char const *, size_t len, bool url = false);

#if __cplusplus >= 201703L
    //
    // Interface with std::string_view rather than const std::string&
    // Requires C++17
    // Provided by Yannic Bonenberger (https://github.com/Yannic)
    //
    std::string base64_encode(std::string_view s, bool url = false);
    std::string base64_encode_pem(std::string_view s);
    std::string base64_encode_mime(std::string_view s);

    std::string base64_decode(std::string_view s, bool remove_linebreaks = false);
#endif // __cplusplus >= 201703L

    //
    // Depending on the url parameter in base64_chars, one of
    // two sets of base64 characters needs to be chosen.
    // They differ in their last two characters.
    //
    static const char *base64_chars[2] = {
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789"
        "+/",

        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789"
        "-_"};

    static unsigned int pos_of_char(const unsigned char chr)
    {
        //
        // Return the position of chr within base64_encode()
        //

        if (chr >= 'A' && chr <= 'Z')
            return chr - 'A';
        else if (chr >= 'a' && chr <= 'z')
            return chr - 'a' + ('Z' - 'A') + 1;
        else if (chr >= '0' && chr <= '9')
            return chr - '0' + ('Z' - 'A') + ('z' - 'a') + 2;
        else if (chr == '+' || chr == '-')
            return 62; // Be liberal with input and accept both url ('-') and non-url ('+') base 64 characters (
        else if (chr == '/' || chr == '_')
            return 63; // Ditto for '/' and '_'
        else
            //
            // 2020-10-23: Throw std::exception rather than const char*
            //(Pablo Martin-Gomez, https://github.com/Bouska)
            //
            throw std::runtime_error("Input is not valid base64-encoded data.");
    }

    static std::string insert_linebreaks(std::string str, size_t distance)
    {
        //
        // Provided by https://github.com/JomaCorpFX, adapted by me.
        //
        if (!str.length())
        {
            return "";
        }

        size_t pos = distance;

        while (pos < str.size())
        {
            str.insert(pos, "\n");
            pos += distance + 1;
        }

        return str;
    }

    template <typename String, unsigned int line_length>
    static std::string encode_with_line_breaks(String s)
    {
        return insert_linebreaks(base64_encode(s, false), line_length);
    }

    template <typename String>
    static std::string encode_pem(String s)
    {
        return encode_with_line_breaks<String, 64>(s);
    }

    template <typename String>
    static std::string encode_mime(String s)
    {
        return encode_with_line_breaks<String, 76>(s);
    }

    template <typename String>
    static std::string encode(String s, bool url)
    {
        return base64_encode(reinterpret_cast<const unsigned char *>(s.data()), s.length(), url);
    }

    std::string base64_encode(unsigned char const *bytes_to_encode, size_t in_len, bool url)
    {

        size_t len_encoded = (in_len + 2) / 3 * 4;

        unsigned char trailing_char = url ? '.' : '=';

        //
        // Choose set of base64 characters. They differ
        // for the last two positions, depending on the url
        // parameter.
        // A bool (as is the parameter url) is guaranteed
        // to evaluate to either 0 or 1 in C++ therefore,
        // the correct character set is chosen by subscripting
        // base64_chars with url.
        //
        const char *base64_chars_ = base64_chars[url];

        std::string ret;
        ret.reserve(len_encoded);

        unsigned int pos = 0;

        while (pos < in_len)
        {
            ret.push_back(base64_chars_[(bytes_to_encode[pos + 0] & 0xfc) >> 2]);

            if (pos + 1 < in_len)
            {
                ret.push_back(base64_chars_[((bytes_to_encode[pos + 0] & 0x03) << 4) + ((bytes_to_encode[pos + 1] & 0xf0) >> 4)]);

                if (pos + 2 < in_len)
                {
                    ret.push_back(base64_chars_[((bytes_to_encode[pos + 1] & 0x0f) << 2) + ((bytes_to_encode[pos + 2] & 0xc0) >> 6)]);
                    ret.push_back(base64_chars_[bytes_to_encode[pos + 2] & 0x3f]);
                }
                else
                {
                    ret.push_back(base64_chars_[(bytes_to_encode[pos + 1] & 0x0f) << 2]);
                    ret.push_back(trailing_char);
                }
            }
            else
            {

                ret.push_back(base64_chars_[(bytes_to_encode[pos + 0] & 0x03) << 4]);
                ret.push_back(trailing_char);
                ret.push_back(trailing_char);
            }

            pos += 3;
        }

        return ret;
    }

    template <typename String>
    static std::string decode(String const &encoded_string, bool remove_linebreaks)
    {
        //
        // decode(…) is templated so that it can be used with String = const std::string&
        // or std::string_view (requires at least C++17)
        //

        if (encoded_string.empty())
            return std::string();

        if (remove_linebreaks)
        {

            std::string copy(encoded_string);

            copy.erase(std::remove(copy.begin(), copy.end(), '\n'), copy.end());

            return base64_decode(copy, false);
        }

        size_t length_of_string = encoded_string.length();
        size_t pos = 0;

        //
        // The approximate length (bytes) of the decoded string might be one or
        // two bytes smaller, depending on the amount of trailing equal signs
        // in the encoded string. This approximation is needed to reserve
        // enough space in the string to be returned.
        //
        size_t approx_length_of_decoded_string = length_of_string / 4 * 3;
        std::string ret;
        ret.reserve(approx_length_of_decoded_string);

        while (pos < length_of_string)
        {
            //
            // Iterate over encoded input string in chunks. The size of all
            // chunks except the last one is 4 bytes.
            //
            // The last chunk might be padded with equal signs or dots
            // in order to make it 4 bytes in size as well, but this
            // is not required as per RFC 2045.
            //
            // All chunks except the last one produce three output bytes.
            //
            // The last chunk produces at least one and up to three bytes.
            //

            size_t pos_of_char_1 = pos_of_char(encoded_string.at(pos + 1));

            //
            // Emit the first output byte that is produced in each chunk:
            //
            ret.push_back(static_cast<std::string::value_type>(((pos_of_char(encoded_string.at(pos + 0))) << 2) + ((pos_of_char_1 & 0x30) >> 4)));

            if ((pos + 2 < length_of_string) && // Check for data that is not padded with equal signs (which is allowed by RFC 2045)
                encoded_string.at(pos + 2) != '=' &&
                encoded_string.at(pos + 2) != '.' // accept URL-safe base 64 strings, too, so check for '.' also.
            )
            {
                //
                // Emit a chunk's second byte (which might not be produced in the last chunk).
                //
                unsigned int pos_of_char_2 = pos_of_char(encoded_string.at(pos + 2));
                ret.push_back(static_cast<std::string::value_type>(((pos_of_char_1 & 0x0f) << 4) + ((pos_of_char_2 & 0x3c) >> 2)));

                if ((pos + 3 < length_of_string) &&
                    encoded_string.at(pos + 3) != '=' &&
                    encoded_string.at(pos + 3) != '.')
                {
                    //
                    // Emit a chunk's third byte (which might not be produced in the last chunk).
                    //
                    ret.push_back(static_cast<std::string::value_type>(((pos_of_char_2 & 0x03) << 6) + pos_of_char(encoded_string.at(pos + 3))));
                }
            }

            pos += 4;
        }

        return ret;
    }

    std::string base64_decode(std::string const &s, bool remove_linebreaks)
    {
        return decode(s, remove_linebreaks);
    }

    std::string base64_encode(std::string const &s, bool url)
    {
        return encode(s, url);
    }

    std::string base64_encode_pem(std::string const &s)
    {
        return encode_pem(s);
    }

    std::string base64_encode_mime(std::string const &s)
    {
        return encode_mime(s);
    }

#if __cplusplus >= 201703L
    //
    // Interface with std::string_view rather than const std::string&
    // Requires C++17
    // Provided by Yannic Bonenberger (https://github.com/Yannic)
    //

    std::string base64_encode(std::string_view s, bool url)
    {
        return encode(s, url);
    }

    std::string base64_encode_pem(std::string_view s)
    {
        return encode_pem(s);
    }

    std::string base64_encode_mime(std::string_view s)
    {
        return encode_mime(s);
    }

    std::string base64_decode(std::string_view s, bool remove_linebreaks)
    {
        return decode(s, remove_linebreaks);
    }

#endif // __cplusplus >= 201703L

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

        std::string result = str.substr(startPos, endPos - startPos);
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
                keyValue.push_back(parameterPair.substr(pos + 1, parameterPair.length() - pos-1));
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
                if (parameters[i][0] != params[i])
                {
                    throw std::runtime_error("Parameters not found");
                }
            }
            for (int i = 0; i < parameters.size(); i++) storesList<<parameters[i][0]+":"<<parameters[i][1]<<std::endl;
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