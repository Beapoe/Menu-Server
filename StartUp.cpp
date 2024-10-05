#include "Server.hpp"

BOOL WINAPI CTRLHandler(DWORD sig)
{
    if (sig == CTRL_C_EVENT)
    {
        std::cout << "destory executed" << std::endl;
        server::pool.setStop(true);
        int before = server::pool.getLeftTasksAmount();
        std::cout << "left tasks: " << before << std::endl;
        while (server::pool.getLeftTasksAmount() != 0)
        {
            if (before != server::pool.getLeftTasksAmount())
            {
                std::cout << "left tasks: " << server::pool.getLeftTasksAmount() << std::endl;
            }
        }
        return TRUE;
    }else return FALSE;
}

int main()
{
    if (SetConsoleCtrlHandler(CTRLHandler, TRUE))
    {
        SOCKET server_socket = server::init(1024);
        while (true)
        {
            SOCKET client_socket = server::CreateConnection(server_socket);
            std::cout << server::getUrl(server::getRequest(client_socket)) << std::endl;
        }
    }
    else
    {
        throw std::runtime_error("SetConsoleCtrlHandler failed");
    }
    return 0;
}