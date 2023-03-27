#ifndef Server_HPP
# define Server_HPP
# include <iostream>
# include <string>
# include <vector>
# include <algorithm>

# include "IO_Manager.hpp"
# include "sys_calls_warp_arounds.hpp"
# include "ClientConnexionHandler.hpp"
# include "HTTP_Message.hpp"

# define MAX_QUEUE_SIZE 10
# define RECV_BUFFER_SIZE 1000
# define LISTENING_PORT 8080

class Server
{
    public:
    //constructors and destructors
    Server() {}
    Server(const Server& other)
    {
        *this = other;
    }
    ~Server() {}
    //operator overloads
    Server& operator=(const Server& rhs)
    {
        (void)rhs;
        return *this;
    }

    //methods
    void accept_client_connexion(int listening_socket_fd)
    {
        int connexion_socket_fd = ws_accept(listening_socket_fd, NULL, NULL, "accepting connexion... da");
        std::cout << "Accepted new client connexion, connexion_socket_fd: " << connexion_socket_fd << '.' << std::endl;
        IO_Manager::set_interest(connexion_socket_fd, EPOLLIN, new ClientConnexionHandler(connexion_socket_fd));
    }

    //Creates a socket, binds it, listens to it and monitor it.
    void setup_connexion_queue(int port)
    {
        //create socket
        int listen_socket_fd = ws_socket(AF_INET, SOCK_STREAM, 0, "creating listening socket");
        //set socket to non blocking
        // ws_fcntl(listen_socket_fd, F_SETFL, O_NONBLOCK);
        //set sock option SO_REUSEADDR to prevent TCP related error 'Adress already in use' when restarting
        int dump = 1;
        ws_setsockopt(listen_socket_fd, SOL_SOCKET, SO_REUSEADDR, &dump, sizeof(int), "");
        //binding of socket to port
        sockaddr_in sock_addr_for_connexion;
        //Using ft_memset before setting the variables we care about abeginllows us to set all the variable we don't care about to zero in one line.
        memset(&sock_addr_for_connexion, 0, sizeof(sockaddr_in));
        sock_addr_for_connexion.sin_family = AF_INET;                       //define adress family, in this case, IPv4
        sock_addr_for_connexion.sin_addr.s_addr = htonl(INADDR_ANY);        //define IP adress, in this case, localhost
        sock_addr_for_connexion.sin_port = htons(port);                     //define port
        ws_bind(listen_socket_fd, (const struct sockaddr *)&sock_addr_for_connexion, sizeof(sockaddr_in), "binding listening socket");
        //start listening for clients connexion requests
        ws_listen(listen_socket_fd, MAX_QUEUE_SIZE, "listen for listening socket... obviously");
        std::cout << "listening..." << std::endl;
        IO_Manager::set_interest<Server>(listen_socket_fd, &Server::accept_client_connexion, NULL, this);
    }

    void Run()
    {
        try
        {
            setup_connexion_queue(LISTENING_PORT);
            IO_Manager::wait_and_call_callbacks();
        }
        catch(std::exception& e)
        {
            std::cerr << e.what() << std::endl;
        }
    }
};
#endif