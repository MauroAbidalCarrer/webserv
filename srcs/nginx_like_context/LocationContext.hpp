#ifndef LocationContext_HPP
# define LocationContext_HPP
# include <iostream>
# include <string>
# include <vector>

# include "typedefs.hpp"
# include "parsing.hpp"

struct LocationContext
{
    string path;
    string root;
    string default_file;
    /*
        first = cgi extension match
        second = cgi launcher(like python3), may be empty if the cgi can be excutated as is
    */
    vector<pair<string, string> > cgi_extensions_and_launchers;
    bool directory_listing;
    vector<string> allowed_methods;

    //constructors and destructors
    LocationContext() : 
    path(), 
    root(), 
    default_file(), 
    cgi_extensions_and_launchers(),
    directory_listing(false),
    allowed_methods()
    {
        allowed_methods.push_back("GET");
        allowed_methods.push_back("POST");
    }
    LocationContext(const LocationContext& other) :
    path(), 
    root(),
    default_file(), 
    cgi_extensions_and_launchers(),
    directory_listing(false)
    {
        *this = other;
    }
    LocationContext(std::string path, string_vec_it_t& it, string_vec_it_t& location_context_end_it) :
    path(), 
    root(),
    default_file(), 
    cgi_extensions_and_launchers(),
    directory_listing(false),
    allowed_methods()
    {
        allowed_methods.push_back("GET");
        allowed_methods.push_back("POST");
        this->path = path;
        while (it != location_context_end_it)
        {
            bool parsed_directive = false;
            parsed_directive |= parsing::set_directive_field("default_file", directive_fields_dsts_t(default_file, NULL), it, location_context_end_it);
            parsed_directive |= parsing::set_directive_field("root", directive_fields_dsts_t(root, NULL), it, location_context_end_it);
            parsed_directive |= parse_cgi_directive(it, location_context_end_it);
            parsed_directive |= parse_directory_listing(it, location_context_end_it);
            parsed_directive |= parse_allow_methods(it, location_context_end_it);
            if (!parsed_directive)
                throw runtime_error("Invalid token in location context in config file: " + *it);
        }
    }
    ~LocationContext() { }
    // //operator overloads
    LocationContext& operator=(const LocationContext& rhs)
    {
        path = rhs.path;
        root = rhs.root;
        default_file = rhs.default_file;
        cgi_extensions_and_launchers = rhs.cgi_extensions_and_launchers;
        directory_listing = rhs.directory_listing;
        allowed_methods = rhs.allowed_methods;
        return *this;
    }
    //methods
    bool parse_cgi_directive(string_vec_it_t& it, string_vec_it_t& server_context_end_it)
    {
        if (*it == "cgi")
        {
            it++;
            if (it == server_context_end_it || *it == ";" || *it == "}" || *it == "{")
                throw std::runtime_error("location context keyword is not followed by a path.");
            pair<string, string> extension_and_launcher;
            extension_and_launcher.first = *it;
            it++;
            if (it == server_context_end_it || *it == "}" || *it == "{")
                throw std::runtime_error("cgi directive is not terminated by a \";\".");
            if (*it != ";")
            {
                extension_and_launcher.second = *it;
                it++;
            }
            if (it == server_context_end_it || *it != ";")
                throw std::runtime_error("cgi directive is not terminated by a \";\".");
            it++;
            // cout << "Adding cgi extension: " << extension_and_launcher.first << ", launcher: " << extension_and_launcher.second << endl;
            cgi_extensions_and_launchers.push_back(extension_and_launcher);
            return true;
        }
        return false;
    }
    static bool parse_location_context(string_vec_it_t& it, string_vec_it_t& server_context_end_it, std::vector<LocationContext>& locationContext_vec)
    {
        if (*it == "location")
        {
            it++;
            if (it == server_context_end_it || *it == ";" || *it == "}" || *it == "{")
                throw std::runtime_error("location context keyword is not followed by a path.");
            std::string path = *it;
            it++;
            if (it == server_context_end_it || *it != "{")
                throw std::runtime_error("location context path is not followed by an openning bracket.");
            string_vec_it_t location_context_end_it = parsing::find_closing_bracket_it(it, server_context_end_it);
            it++;
            locationContext_vec.push_back(LocationContext(path, it, location_context_end_it));
            it++;
            return true;
        }
        return false;
    }
    bool parse_directory_listing(string_vec_it_t& it, string_vec_it_t& location_context_end_it)
    {
        if (*it == "directory_listing")
        {
            it++;
            if (it == location_context_end_it || *it != ";" )
                throw std::runtime_error("\"directory_listing\" directive is not terminated by a \";\".");
            it++;
            directory_listing = true;
            return true;
        }
        return false;
    }
    bool parse_allow_methods(string_vec_it_t& it, string_vec_it_t& location_context_end_it)
    {
        if (*it == "allowed_methods")
        {
            allowed_methods.clear();
            it++;
            while (*it == "GET" || *it == "DELETE" || *it == "POST" )
            {
                allowed_methods.push_back(*it);
                it++;
            }
            if (it == location_context_end_it || *it != ";")
                throw std::runtime_error("\"allow_methods\" directive is not terminated by a \";\".");
            it++;
            return true;
        }
        return false;
    }

    void apply_to_path(string& path)
    {
		// apply root directive(for now just insert ".")
		path = root + path;
		// apply rewrite directive(not sure if it's the rewrite directive... the that completes target URLs finishing ini "/")(for no just index.html)
		if (*(path.end() - 1) == '/')
			path.append(default_file);
# ifndef NO_DEBUG
		cout << "Path after applying context: " << path << endl;
# endif
    }
    void debug()
    {
        cout << "\t\tlocation path: " << path << endl;
        cout << "\t\troot: " << root << endl;
        cout << "\t\tdefault_file: " << default_file << endl;  
        cout << "\t\tdirectory_listing: " << (directory_listing ? "true" : "false") << endl;
        cout << "\t\tcgi extensions and launchers(optional):" << endl;
        for (size_t i = 0; i < cgi_extensions_and_launchers.size(); i++)
            cout << "\t\t\t" << cgi_extensions_and_launchers[i].first << " " << cgi_extensions_and_launchers[i].second << endl;
        cout << "\t\tAllowed methods:";
        for (size_t i = 0; i < allowed_methods.size(); i++)
            cout << " " << allowed_methods[i];
        cout << endl;
    }
};
#endif