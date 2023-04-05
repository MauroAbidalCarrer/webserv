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
    //constructors and destructors
    // LocationContext() { }
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
            if (!parsed_directive)
                throw runtime_error("Invalid token in location context in config file.");
        }
    }
    ~LocationContext() { }
    // //operator overloads
    LocationContext& operator=(const LocationContext& rhs)
    {
        root = rhs.root;
        default_file = rhs.default_file;
        return *this;
    }
    //methods

    static bool parse_location_context(string_vec_it_t it, string_vec_it_t& server_context_end_it, std::vector<LocationContext>& locationContext_vec)
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
            locationContext_vec.push_back(LocationContext(location_path, it, location_context_end_it));
            return true;
        }
        return false;
    }
};
#endif