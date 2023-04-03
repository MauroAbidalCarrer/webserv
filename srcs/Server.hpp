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
# include "typedefs.hpp"

# define MAX_QUEUE_SIZE 10
# define RECV_BUFFER_SIZE 1000
# define LISTENING_PORT 8080

# define CSVS_DIR_PATH "internal_server_ressources/CSVs/"
extern std::map<std::string, std::map<std::string, std::string> > CSV_maps;

class Server
{
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
    void Run()
    {
        try
        {
            setup_CSV_maps();
            setup_connexion_queue(LISTENING_PORT);
            IO_Manager::wait_and_call_callbacks();
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << std::endl;
        }
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
    void accept_client_connexion(int listening_socket_fd)
    {
        int connexion_socket_fd = ws_accept(listening_socket_fd, NULL, NULL, "accepting connexion... da");
        std::cout << "Accepted new client connexion, connexion_socket_fd: " << connexion_socket_fd << '.' << std::endl;
        IO_Manager::set_interest(connexion_socket_fd, EPOLLIN, new ClientConnexionHandler(connexion_socket_fd));
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