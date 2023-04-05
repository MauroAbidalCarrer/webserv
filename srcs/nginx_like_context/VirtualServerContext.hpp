#ifndef ServerContext_HPP
# define ServerContext_HPP
# include <iostream>
# include <string>
# include <algorithm>

# include "typedefs.hpp"
# include "LocationContext.hpp"


//Whenever "server" is mentioned it is implied that it is "virtual server"

struct ServerContext
{
    //listen fields
    string listen_adress, listen_port;
    vector<string> server_hostnames;
    str_to_str_map_t error_codes_to_redirect_URLS;
    vector<LocationContext> location_contexts;
    
    //constructors and destructors
    ServerContext(string_vec_it_t it, string_vec_it_t server_context_end_it)
    {
        listen_adress = "127.0.0.1";
        listen_port = "80";
        while (it != server_context_end_it)
        {
            bool parsed_a_directive = false;
            parsed_a_directive |= LocationContext::parse_location_context(it, server_context_end_it, location_contexts);
            parsed_a_directive |= parse_listen_directive(it, server_context_end_it);
            parsed_a_directive |= parse_error_page_directive(it, server_context_end_it);
            parsed_a_directive |= parse_server_name_directive(it, server_context_end_it);
            if (!parsed_a_directive)
                throw std::runtime_error("invalid token in virtual server context : " + *it);
        }
    }
    //returns true if a server_name drective was parced
    bool parse_server_name_directive(string_vec_it_t& it, string_vec_it_t server_context_end_it)
    {
        std::string server_hostname_buffer;
        if (parsing::set_directive_field("server_name", directive_fields_dsts_t(server_hostname_buffer, NULL), it, server_context_end_it))
        {
            server_hostnames.push_back(server_hostname_buffer);
            return true;
        }
        return false;
    }
    //returns true if an error drective was parced
    bool parse_error_page_directive(string_vec_it_t& it, string_vec_it_t server_context_end_it)
    {
        std::string error_status_code_buffer, redirect_URL_buffer;
        if (parsing::set_directive_field("error_page", directive_fields_dsts_t(error_status_code_buffer, &redirect_URL_buffer), it, server_context_end_it))
        {
            error_codes_to_redirect_URLS[error_status_code_buffer] = redirect_URL_buffer;
            return true;
        }
        return false;
    }
    //returns true if a listen directive was parced
    bool parse_listen_directive(string_vec_it_t& it, string_vec_it_t server_context_end_it)
    {
        std::string listen_buffer;
        if (parsing::set_directive_field("listen", directive_fields_dsts_t(listen_buffer, NULL), it, server_context_end_it))
        {
            size_t colon_pos = listen_buffer.find(':');
            if (colon_pos == listen_buffer.size())
                throw runtime_error("There is a colon in a listen field but there is no post after.");
            if (colon_pos != std::string::npos)
            {
                listen_adress = listen_buffer.substr(0, colon_pos);
                listen_port = listen_buffer.substr(colon_pos + 1);
            }
            else
            {
                int nb_dots = std::count(listen_buffer.begin(), listen_buffer.end(), '.');
                //if the listen value contains three dots, it is a ip address
                if (nb_dots == 3)
                    listen_adress = listen_buffer;
                //otherwise, it's (most likely) a port
                else
                    listen_port = listen_buffer;
            }
            return true;
        }
        return false;
    }
    ServerContext(const ServerContext& other)
    {
        *this = other;
    }
    ~ServerContext() { }
    //operator overloads
    ServerContext& operator=(const ServerContext& rhs)
    {
        listen_adress = rhs.listen_adress;
        listen_port = rhs.listen_port;
        server_hostnames = rhs.server_hostnames;
        error_codes_to_redirect_URLS = rhs.error_codes_to_redirect_URLS;
        location_contexts = rhs.location_contexts;
        return *this;
    }
    //methods
    void debug()
    {
        cout << "virtual server context:" << endl;
        cout << "\tlisten_adress\t: " << listen_adress << endl;
        cout << "\tlisten_port\t: " << listen_port << endl;
        cout << "\tserver hostnames: ";
        for (size_t i = 0; i < server_hostnames.size(); i++)
            cout << server_hostnames[i] << endl;
        cout << "\terror pages redirects:" << endl;
        for (str_to_str_map_t::iterator it = error_codes_to_redirect_URLS.begin(); it != error_codes_to_redirect_URLS.end(); it++)
            cout << "\t\t" << it->first << " => " << it->second << endl;
        cout << "\tlocation contexts: " << endl;
        for (size_t i = 0; i < location_contexts.size(); i++)
            location_contexts[i].debug();
    }
};
#endif