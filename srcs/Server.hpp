#ifndef Server_HPP
# define Server_HPP
# include <iostream>
# include <string>
# include <vector>
# include <algorithm>
# include <netdb.h>
# include <sys/socket.h>
# include <netinet/in.h>
# include <arpa/inet.h>

# include "IO_Manager.hpp"
# include "sys_calls_warp_arounds.hpp"
# include "ClientConnexionHandler.hpp"
# include "HTTP_Message.hpp"
# include "typedefs.hpp"
# include "VirtualServerContext.hpp"
# include "GlobalContext.hpp"

# define MAX_QUEUE_SIZE 10
# define RECV_BUFFER_SIZE 1000
# define LISTENING_PORT 8080

# define CSVS_DIR_PATH "internal_server_ressources/CSVs/"
extern CSV_maps_t CSV_maps;

extern GlobalContext GlobalContextSingleton;

class Server
{
    private:
    //fields
    
    public:
    //constructors and destructors
    Server() {}
    Server(const Server& other)
    {
        *this = other;
    }
    ~Server() { }
    //operator overloads
    Server& operator=(const Server& rhs)
    {
        (void)rhs;
        return *this;
    }

    //methods
    void Run(string config_file_path)
    {
        try
        {
            setup_CSV_maps();
            GlobalContextSingleton = GlobalContext(config_file_path);
            setup_connexion_queues();
            IO_Manager::wait_and_call_callbacks();
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << std::endl;
        }
    }

    typedef struct addrinfo addrinfo_t;
    typedef struct sockaddr sockaddr_t;
    void setup_connexion_queues()
    {
        vector<pair<string, string> > network_interfaces_already_used;
        for (size_t i = 0; i < GlobalContextSingleton.virtual_server_contexts.size(); i++)
        {
            VirtualServerContext virtual_server_context = GlobalContextSingleton.virtual_server_contexts[i];
            //get address info
            string ip = virtual_server_context.listen_adress;
            string port = virtual_server_context.listen_port;
            pair<string, string> network_interface = std::make_pair(ip, port);
            if (std::count(network_interfaces_already_used.begin(), network_interfaces_already_used.end(), network_interface))
            {
                // cout << "Already listenning to network interface " << ip << ":" << port << endl;
                continue;
            }
            // cout << "Trying to listen to any of the network interfaces for " << ip << ":" << port << endl;
            setup_connexion_queue(ip, port);
            network_interfaces_already_used.push_back(network_interface);
        }
    }
    void setup_connexion_queue(string ip, string port)
    {
        addrinfo_t* addr_info = get_addrinfo(ip, port);
        addrinfo_t *addr_info_ptr = addr_info;
        for (; addr_info_ptr != NULL; addr_info_ptr = addr_info_ptr->ai_next)
        {
            try
            {
                // cout << "\tTrying to listen on ";
                // print_socket_address_and_port(addr_info_ptr);
                // cout << endl;
                setup_connexion_queue(*addr_info_ptr);
            }
            catch(const std::exception& e)
            {
                //catched an exception while trying to setup connexion queue, try to set it up with next addr info
                continue ;
            }
            //Did not catch any error(otherwise we would of continued), which means that the listening socket has succefully been succefully set up.
            break;
        }
        freeaddrinfo(addr_info);
        //if we reached the end of the possible address info list it means that it is not possible to listen to 
        // cout << "addr_info_ptr at end of for loop = " << addr_info_ptr << endl;
        if (addr_info_ptr == NULL)
        {
            string error_msg = string(RED_AINSI) + "Error" + END_AINSI + ": Could not listen to any of the adress infos related to " + ip + ":" + port;
            throw runtime_error(error_msg);
        }
    }
    //Creates a socket, binds it, listens to it and monitors it.
    void setup_connexion_queue(addrinfo_t addr_info)
    {
        //create socket
        int listen_socket_fd = ws_socket(AF_INET, SOCK_STREAM, 0);
        //set socket to non blocking
        // ws_fcntl(listen_socket_fd, F_SETFL, O_NONBLOCK);
        //set sock option SO_REUSEADDR to prevent TCP related error 'Adress already in use' when restarting
        int dump = 1;
        ws_setsockopt(listen_socket_fd, SOL_SOCKET, SO_REUSEADDR, &dump, sizeof(int));
        //binding of socket to port
        ws_bind(listen_socket_fd, addr_info.ai_addr, addr_info.ai_addrlen);
        //start listening for clients connexion requests
        ws_listen(listen_socket_fd, MAX_QUEUE_SIZE);
        IO_Manager::set_interest(listen_socket_fd, &Server::accept_client_connexion, NULL);
        //debugging
        cout << "listening to ";
        print_socket_address_and_port(addr_info.ai_addr);
        cout << endl;
    }
    struct addrinfo* get_addrinfo(string ip, string port)
    {
        struct addrinfo hints;
        memset(&hints, 0, sizeof hints);
        hints.ai_family = AF_UNSPEC;        //accept any familly of IP address(IPV4 or IPV6)
        hints.ai_socktype = SOCK_STREAM;    //set transport protocol to TCP
        struct addrinfo* addr_info_lst;

        int result = getaddrinfo(ip.data(), port.data(), &hints, &addr_info_lst);
        if (result != 0) 
        {
            string error_msg = "getaddrinfo failed (" + ip + ":" + port + "): " + string(gai_strerror(result));
            throw runtime_error(error_msg);
        }
        //according to the man, addr_info_lst contains the list of possible address for the pair give ip:port pair... I believe
        return addr_info_lst;
    }
    static void print_socket_address_and_port(sockaddr_t *ai_addr)
    {
        // Cast the socket address information to a sockaddr_in or sockaddr_in6
        // structure depending on the address family.
        if (ai_addr->sa_family == AF_INET) 
        {
            // IPv4 address
            struct sockaddr_in* ipv4_socket_addr = reinterpret_cast<struct sockaddr_in*>(ai_addr);
            // Access the IP address and port from the sockaddr_in structure.
            cout << inet_ntoa(ipv4_socket_addr->sin_addr) << ":" << ntohs(ipv4_socket_addr->sin_port);
        }
        else
        {
            // IPv6 address
            struct sockaddr_in6* ipv6_socket_addr = reinterpret_cast<struct sockaddr_in6*>(ai_addr);
            // Access the IP address and port from the sockaddr_in6 structure.
            char ip_str[INET6_ADDRSTRLEN];
            inet_ntop(AF_INET6, &(ipv6_socket_addr->sin6_addr), ip_str, INET6_ADDRSTRLEN);
            cout << ip_str << ":"  << ntohs(ipv6_socket_addr->sin6_port);
        }
    }
    static void accept_client_connexion(int listening_socket_fd)
    {
        int connexion_socket_fd = ws_accept(listening_socket_fd, NULL, NULL);
        IO_Manager::set_interest(connexion_socket_fd, EPOLLIN, new ClientConnexionHandler(connexion_socket_fd));
        //debugging
        cout << "New client connexion on socket " << connexion_socket_fd << '.' << endl;
        struct sockaddr_storage local_addr;

        socklen_t addr_len = sizeof(local_addr);
        if (getsockname(listening_socket_fd, (struct sockaddr*)&local_addr, &addr_len) == -1)
        {
            cout << "Failed to getsockname: " << strerror(errno) << endl;
            return;
        }
        // Determine the address family (IPv4 or IPv6)
        if (local_addr.ss_family == AF_INET) {
            struct sockaddr_in *addr_in = (struct sockaddr_in *)&local_addr;
            cout << "Listening socket is binded to IPV4 network interface: ";
            print_socket_address_and_port((sockaddr_t *)addr_in);
            cout << endl;
        }
        else if (local_addr.ss_family == AF_INET6)
        {
            struct sockaddr_in6 *addr_in6 = (struct sockaddr_in6 *)&local_addr;
            cout << "Listening socket is binded to IPV6 network interface: ";
            print_socket_address_and_port((sockaddr_t *)addr_in6);
            cout << endl;
        }
        else
            cerr << "Error while getting socket address\n";
    }

    void setup_CSV_maps()
    {
        string_vec_t CSVs_filenames = list_files_in_directory(CSVS_DIR_PATH);
        for (size_t i = 0; i < CSVs_filenames.size(); i++)
        {
            std::string CSVs_filename = CSVS_DIR_PATH + CSVs_filenames[i];
            std::string CSV_content_as_text = read_file_into_string(CSVs_filename);
            parsing::tokenized_text_t tokenized_CSV = parsing::tokenize_text(CSV_content_as_text, "\n", ",");
            str_to_str_map_t CSV_map;
            for (size_t j = 0; j < tokenized_CSV.size(); j++)
            {
                string_vec_t line = tokenized_CSV[j];
                if (line.size() >= 2 && !line[0].empty() && !line[1].empty() && line[0] != "Name")
                {
                    // std::cout << "adding key = " << line[0] << ", value = " << line[1] << std::endl;
                    CSV_map[line[0]] = line[1];
                }
            }
            std::string CSV_filname_without_extension = CSVs_filenames[i].substr(0, CSVs_filenames[i].find(".csv"));
            CSV_maps[CSV_filname_without_extension] = CSV_map;
        }
    }
};
#endif