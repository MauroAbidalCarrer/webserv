#ifndef ServerContext_HPP
# define ServerContext_HPP
# include <iostream>
# include <string>
# include <algorithm>

# include "typedefs.hpp"
# include "LocationContext.hpp"

# define MAX_BODY_SIZE_LIMIT 300000000

//Whenever "server" is mentioned it is implied that it is "virtual server"

struct VirtualServerContext
{
    //listen fields
    string listen_ip, listen_port;
    vector<string> hostnames;
    str_to_str_map_t error_codes_to_default_error_page_path;
    vector<LocationContext> location_contexts;
    map<string, string> redirected_URLs;
    string client_body_size_limit_as_string;
    size_t client_body_size_limit;
    
    //constructors and destructors
    VirtualServerContext() :
    listen_ip(),
    listen_port(),
    hostnames(),
    error_codes_to_default_error_page_path(),
    location_contexts(),
    redirected_URLs(),
    client_body_size_limit_as_string(),
    client_body_size_limit(MAX_BODY_SIZE_LIMIT)
    { }
    VirtualServerContext(string_vec_it_t it, string_vec_it_t server_context_end_it) :
    listen_ip(),
    listen_port(),
    hostnames(),
    error_codes_to_default_error_page_path(),
    location_contexts(),
    redirected_URLs(),
    client_body_size_limit_as_string(),
    client_body_size_limit(MAX_BODY_SIZE_LIMIT)
    {
        listen_ip = "127.0.0.1";
        listen_port = "80";
        while (it != server_context_end_it)
        {
            bool parsed_a_directive = false;
            parsed_a_directive |= LocationContext::parse_location_context(it, server_context_end_it, location_contexts);
            parsed_a_directive |= parse_listen_directive(it, server_context_end_it);
            parsed_a_directive |= parse_error_page_directive(it, server_context_end_it);
            parsed_a_directive |= parse_server_name_directive(it, server_context_end_it);
            parsed_a_directive |= parse_redirections(it, server_context_end_it);
            parsed_a_directive |= parsing::set_directive_field("client_body_size_limit", directive_fields_dsts_t(client_body_size_limit_as_string, NULL), it, server_context_end_it);
            if (!parsed_a_directive)
                throw std::runtime_error("invalid token in virtual server context : " + *it);
        }
        if (listen_ip == "localhost")
            listen_ip = "127.0.0.1";
        if (client_body_size_limit_as_string.empty() == false)
            client_body_size_limit = std::strtoul(client_body_size_limit_as_string.c_str(), NULL, 0);
        if (client_body_size_limit > MAX_BODY_SIZE_LIMIT)
            client_body_size_limit = MAX_BODY_SIZE_LIMIT;
    }
    //returns true if a server_name drective was parced
    bool parse_server_name_directive(string_vec_it_t& it, string_vec_it_t server_context_end_it)
    {
        std::string server_hostname_buffer;
        if (parsing::set_directive_field("server_name", directive_fields_dsts_t(server_hostname_buffer, NULL), it, server_context_end_it))
        {
            hostnames.push_back(server_hostname_buffer);
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
            error_codes_to_default_error_page_path[error_status_code_buffer] = redirect_URL_buffer;
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
                listen_ip = listen_buffer.substr(0, colon_pos);
                listen_port = listen_buffer.substr(colon_pos + 1);
            }
            else
            {
                int nb_dots = std::count(listen_buffer.begin(), listen_buffer.end(), '.');
                //if the listen value contains three dots, it is a ip address
                if (nb_dots == 3)
                    listen_ip = listen_buffer;
                //otherwise, it's (most likely) a port
                else
                    listen_port = listen_buffer;
            }
            return true;
        }
        return false;
    }
    VirtualServerContext(const VirtualServerContext& other)
    {
        *this = other;
    }
    ~VirtualServerContext() { }
    //operator overloads
    VirtualServerContext& operator=(const VirtualServerContext& rhs)
    {
        listen_ip = rhs.listen_ip;
        listen_port = rhs.listen_port;
        hostnames = rhs.hostnames;
        error_codes_to_default_error_page_path = rhs.error_codes_to_default_error_page_path;
        location_contexts = rhs.location_contexts;
        redirected_URLs = rhs.redirected_URLs;
        client_body_size_limit = rhs.client_body_size_limit;
        return *this;
    }
    //methods
    void debug()
    {
        cout << BOLD_AINSI << "Virtual server context:" << END_AINSI << endl;
        cout << "\tlisten_adress\t: " << listen_ip << endl;
        cout << "\tlisten_port\t: " << listen_port << endl;
        cout << "\tclient_body_size_limit: " << client_body_size_limit << endl;
        cout << "\tserver hostnames: ";
        for (size_t i = 0; i < hostnames.size(); i++)
            cout << hostnames[i] << endl;
        cout << "\terror pages redirects:" << endl;
        for (str_to_str_map_t::iterator it = error_codes_to_default_error_page_path.begin(); it != error_codes_to_default_error_page_path.end(); it++)
            cout << "\t\t" << it->first << " => " << it->second << endl;
        cout << "\tredirections:" << endl;
        for (map<string, string>::iterator it = redirected_URLs.begin(); it != redirected_URLs.end(); it++)
            cout << "\t\t" << it->first << " => " << it->second << endl;
        cout << BOLD_AINSI << "\tlocation contexts: " << END_AINSI << endl;
        for (size_t i = 0; i < location_contexts.size(); i++)
            location_contexts[i].debug();
    }
    const LocationContext& find_corresponding_location_context(string path)
    {
        LocationContext* best_location_context_match = NULL;//location with highest char match count
        size_t best_nb_char_match = 0;
        for (vector<LocationContext>::iterator location_it = location_contexts.begin(); location_it != location_contexts.end(); location_it++)
        {
            if (string_starts_by(path, location_it->path) && location_it->path.length() > best_nb_char_match)
            {
                best_location_context_match = &(*location_it);
                best_nb_char_match = location_it->path.length();
            }
        }
        if (best_location_context_match == NULL)
            throw WSexception("400", "Could not found server context for path \"" + path + "\"");
        return *best_location_context_match;
    }
    bool string_starts_by(string a, string b)
    {
		if (a.length() >= b.length())
			return (a.compare(0, b.length(), b) == 0);
		return false;
    }
    bool parse_redirections(string_vec_it_t& it, string_vec_it_t server_context_end_it)
    {
        if (*it == "redirect")
        {
            cout << "Found \"redirect\" directive." << endl;
            it++;
            if (it == server_context_end_it || *it == ";" || *it == "}" || *it == "{") 
                throw runtime_error("\"redirect\" directive is not followed by a url key.");
            it++;
            if (it == server_context_end_it || *it == ";" || *it == "}" || *it == "{") 
                throw runtime_error("\"redirect\" directive url key is not followed by a url value.");
            redirected_URLs[*(it - 1)] = *it;
            it++;
            if (it == server_context_end_it || *it != ";")
                throw runtime_error("redirect directive does not end by a \";\".");
            it++;
            return true;
        }
        return false;
    }
};
#endif