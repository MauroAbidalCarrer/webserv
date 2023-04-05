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
# include "VirtualServerContext.hpp"

# define MAX_QUEUE_SIZE 10
# define RECV_BUFFER_SIZE 1000
# define LISTENING_PORT 8080

# define CSVS_DIR_PATH "internal_server_ressources/CSVs/"
extern CSV_maps_t CSV_maps;

class Server
{
    private:
    //fields
    vector<ServerContext> virtual_server_contexts;
    
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
        (void)config_file_path;
        try
        {
            setup_CSV_maps();
            // setup_virtual_server_contexts(config_file_path);
            setup_connexion_queue(LISTENING_PORT);
            IO_Manager::wait_and_call_callbacks();
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << std::endl;
        }
    }

    //Creates a socket, binds it, listens to it and monitors it.
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
        //Using ft_memset before setting the variables we care about at the begining allows us to set all the variable we don't care about to zero in one line.
        memset(&sock_addr_for_connexion, 0, sizeof(sockaddr_in));
        sock_addr_for_connexion.sin_family = AF_INET;                       //define adress family, in this case, IPv4
        sock_addr_for_connexion.sin_addr.s_addr = htonl(INADDR_ANY);        //define IP adress, in this case, localhost
        sock_addr_for_connexion.sin_port = htons(port);                     //define port
        ws_bind(listen_socket_fd, (const struct sockaddr *)&sock_addr_for_connexion, sizeof(sockaddr_in), "binding listening socket");
        //start listening for clients connexion requests
        ws_listen(listen_socket_fd, MAX_QUEUE_SIZE, "listen for listening socket... obviously");
        IO_Manager::set_interest<Server>(listen_socket_fd, &Server::accept_client_connexion, NULL, this);
        //debugging
        std::cout << "listening..." << std::endl;
    }
    void accept_client_connexion(int listening_socket_fd)
    {
        int connexion_socket_fd = ws_accept(listening_socket_fd, NULL, NULL, "accepting connexion... da");
        IO_Manager::set_interest(connexion_socket_fd, EPOLLIN, new ClientConnexionHandler(connexion_socket_fd));
        //debugging
        std::cout << "Accepted new client connexion, connexion_socket_fd: " << connexion_socket_fd << '.' << std::endl;
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

    void setup_virtual_server_contexts(std::string config_file_path)
    {
        vector<string> config_file_tokens = tokenize_config_file(config_file_path);
        string_vec_it_t end_it = config_file_tokens.end();
        for (string_vec_it_t it = config_file_tokens.begin(); it != end_it; it++)
        {
            if (*it != "server")
                throw runtime_error("invalid token in global context " + *it + "(expected \"server\").");
            it++;
            if (it == end_it || *it != "{")
                throw std::runtime_error("virtual server context path is not followed by an openning bracket.");
            string_vec_it_t server_context_end_it = parsing::find_closing_bracket_it(it, end_it);
            virtual_server_contexts.push_back(ServerContext(it, server_context_end_it));
            it = server_context_end_it;
        }
    }
    vector<string> tokenize_config_file(std::string config_file_path)
    {
        string config_file_content = read_file_into_string(config_file_path);
        vector<string> config_file_tokens = parsing::tokenize_first_of(config_file_content, " \n\t\v\r", false);
        for (size_t i = 0; i < config_file_tokens.size(); i++)
        {
            string& token = config_file_tokens[i];
            if (token.size() > 1 && token[token.size() - 1] == ';')
            {
                token.erase(token.size() - 1);
                config_file_tokens.insert(config_file_tokens.begin() + i + 1, ";");
            }
        }
        for (size_t i = 0; i < config_file_tokens.size(); i++)
        {
            std::cout << config_file_tokens[i] /* << ", " */ << std::endl;
        }
        return config_file_tokens;
    }

};
#endif