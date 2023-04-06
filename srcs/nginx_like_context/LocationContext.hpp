#ifndef LocationContext_HPP
# define LocationContext_HPP
# include <iostream>
# include <string>
# include <vector>

# include "typedefs.hpp"
# include "parsing.hpp"

struct LocationContext
{
    string location_path;
    string root;
    string default_file;
    /*
        first = cgi extension match
        second = cgi launcher(like python3), may be empty if the cgi can be excutated as is
    */
    vector<pair<string, string> > cgi_extensions_and_launchers;

    //constructors and destructors
    LocationContext() { }
    LocationContext(const LocationContext& other)
    {
        *this = other;
    }
    LocationContext(std::string location_path, string_vec_it_t& it, string_vec_it_t& location_context_end_it)
    {
        this->location_path = location_path;
        // vector<string> keywords;
        // keywords.push_back("root");
        // keywords.push_back("default_file");
        // vector<directive_fields_dsts_t> directive_field_dsts;
        // directive_field_dsts.push_back(directive_fields_dsts_t(root, NULL));
        // directive_field_dsts.push_back(directive_fields_dsts_t(default_file, NULL));
        // while (it != location_context_end_it)
        // {
        //     //if no valid directive was found, it means that the token the iterator is pointing to is invalid
        //     if (!parsing::set_directive_fields(keywords, directive_field_dsts, it, location_context_end_it))
        //         throw std::runtime_error("invalid token in configuration file: " + *it);
        // }
        while (it != location_context_end_it)
        {
            bool parsed_directive = false;
            parsed_directive |= parsing::set_directive_field("default_file", directive_fields_dsts_t(default_file, NULL), it, location_context_end_it);
            parsed_directive |= parsing::set_directive_field("root", directive_fields_dsts_t(root, NULL), it, location_context_end_it);
            parsed_directive |= parse_cgi_directive(it, location_context_end_it);
            if (!parsed_directive)
                throw runtime_error("Invalid token in location context in config file: " + *it);
        }
    }
    ~LocationContext() { }
    // //operator overloads
    LocationContext& operator=(const LocationContext& rhs)
    {
        location_path = rhs.location_path;
        root = rhs.root;
        default_file = rhs.default_file;
        cgi_extensions_and_launchers = rhs.cgi_extensions_and_launchers;
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
            cout << "Adding cgi extension: " << extension_and_launcher.first << ", launcher: " << extension_and_launcher.second << endl;
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
            std::string location_path = *it;
            it++;
            if (it == server_context_end_it || *it != "{")
                throw std::runtime_error("location context path is not followed by an openning bracket.");
            string_vec_it_t location_context_end_it = parsing::find_closing_bracket_it(it, server_context_end_it);
            it++;
            locationContext_vec.push_back(LocationContext(location_path, it, location_context_end_it));
            it++;
            return true;
        }
        return false;
    }
    void debug()
    {
        cout << "\t\tlocation path: " << location_path << endl;
        cout << "\t\troot: " << root << endl;
        cout << "\t\tdefault_file: " << default_file << endl;  
        cout << "\t\tcgi extensions and launchers(optional):" << endl;
        for (size_t i = 0; i < cgi_extensions_and_launchers.size(); i++)
            cout << "\t\t\t" << cgi_extensions_and_launchers[i].first << " " << cgi_extensions_and_launchers[i].second << endl;
    }
};
#endif