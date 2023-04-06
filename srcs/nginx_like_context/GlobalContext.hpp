#ifndef GlobalContext_HPP
# define GlobalContext_HPP
# include <iostream>
# include <string>

# include "typedefs.hpp"
# include "VirtualServerContext.hpp"
# include "ClientConnexionHandler.hpp"

class GlobalContext
{
    public:
    vector<VirtualServerContext> virtual_server_contexts;

    public:
    //constructors and destructors
    GlobalContext() { }
    GlobalContext(const GlobalContext& other)
    {
        *this = other;
    }
    GlobalContext(string config_file_path)
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
            it++;
            virtual_server_contexts.push_back(VirtualServerContext(it, server_context_end_it));
            it = server_context_end_it;
        }
        //debugging
        for (size_t i = 0; i < virtual_server_contexts.size(); i++)
            virtual_server_contexts[i].debug();
        cout << endl;
    }
    ~GlobalContext() { }
    //operator overloads
    GlobalContext& operator=(const GlobalContext& rhs)
    {
        virtual_server_contexts = rhs.virtual_server_contexts;
        return *this;
    }
    //methods
    private:
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
        return config_file_tokens;
    }
    public:
    const VirtualServerContext& find_corresponding_virtualServerContext(const HTTP_Request& request, string listen_ip, string listen_port) 
    {
        // vector<VirtualServerContext> candidate_virtual_contexts = virtual_server_contexts;
        // for (size_t i = 0; i < candidate_virtual_contexts.size(); i++)
        // {
        //     const VirtualServerContext& candidate = candidate_virtual_contexts[i];
        //     if (candidate.listen_adress != connexion_handler.listening_ip || candidate.listen_port != connexion_handler.listen_port)
        //     {
        //         candidate_virtual_contexts.erase(candidate_virtual_contexts.begin() + i);
        //         continue;
        //     }
        // }
        // if (candidate_virtual_contexts.size() == )
        (void)request;
        (void)listen_ip;
        (void)listen_port;
        return virtual_server_contexts[0];
    }
};
#endif