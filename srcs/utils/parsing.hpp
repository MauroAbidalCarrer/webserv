#ifndef PARSING 
# define PARSING
# include <string>
# include <vector>
# include <sstream>
# include <iostream>
# include <algorithm>

# include "typedefs.hpp"
# include "sys_calls_warp_arounds.hpp"

namespace parsing
{
    //low level
    string_vec_t tokenize(std::string str, std::string delimiters, bool include_empty_tokens)
    {
        std::vector<std::string> tokens;
        size_t pos = 0;
        while (pos != str.length())
        {
            size_t next_pos = str.find(delimiters, pos);
            if (next_pos == std::string::npos)
            {
                tokens.push_back(str.substr(pos));
                break;
            }
            size_t len = next_pos - pos;
            if (len != 0 || include_empty_tokens)
                tokens.push_back(str.substr(pos, len));
            pos = next_pos + delimiters.length();
        }
        return tokens;
    }
    string_vec_t tokenize_first_of(std::string str, std::string delimiters, bool include_empty_tokens)
    {
        std::vector<std::string> tokens;
        size_t pos = 0;
        while (pos != str.length())
        {
            size_t next_pos = str.find_first_of(delimiters, pos);
            if (next_pos == std::string::npos)
            {
                tokens.push_back(str.substr(pos));
                break;
            }
            size_t len = next_pos - pos;
            if (len != 0 || include_empty_tokens)
                tokens.push_back(str.substr(pos, len));
            pos = next_pos + 1;
        }
        return tokens;
    }

    //HTTP_Message parsing
    const char * CLRF = "\r\n";
    typedef std::vector<string_vec_t> tokenized_text_t;
    tokenized_text_t tokenize_text(std::string file_str, std::string line_delimiter, std::string word_delimiters)
    {
        tokenized_text_t tokenized_file;
        string_vec_t line_tokens = tokenize(file_str, line_delimiter, true);
        for (size_t i = 0; i < line_tokens.size(); i++)
            tokenized_file.push_back(tokenize_first_of(line_tokens[i], word_delimiters, false));
        return tokenized_file;
    }
    typedef tokenized_text_t tokenized_HTTP_t;
    tokenized_HTTP_t tokenize_HTTP_message(std::string message_text)
    {
        return tokenize_text(message_text, CLRF, ": ");
    }

    //configuration file parsing
    //directives parsing
    //returns true if a valid directive was found
    bool set_directive_field(std::string directive_keyword, const directive_fields_dsts_t&  field_dsts, string_vec_it_t& it, const string_vec_it_t& end_it)
    {
        if (it != end_it && *it == directive_keyword)
        {
            it++;
            if (it == end_it || *it == ";")
                throw std::runtime_error("directive \"" + directive_keyword + "\" not followed by a value.");
            // it++;
            field_dsts.first = *it;
            // std::cout << "field_dsts.first = " << field_dsts.first << std::endl;
            if (field_dsts.second != NULL)
            {
                it++;
                if (it == end_it || *it == ";")
                    throw std::runtime_error("directive \"" + directive_keyword + "\" is missing a second value.");
                *field_dsts.second = *it;
            }
            it++;
            if (it == end_it || *it != ";")
                throw std::runtime_error("directive \"" + directive_keyword + "\" field is not terminated by a ';', previous token = " + *(it - 1));
            it++;
            //debugging
            // if (field_dsts.second != NULL)
            //     std::cout << directive_keyword << " = " << field_dsts.first << " " << *field_dsts.second << std::endl;
            // else
            //     std::cout << directive_keyword << " = " << field_dsts.first << std::endl;
            return true;
        }
        return false;
    }
    //returns true if a valid directive was found
    // bool set_directive_fields(string_vec_t directive_keywords, vector<directive_fields_dsts_t> field_dsts, string_vec_it_t& it, string_vec_it_t& end_it)
    // {
    //     bool found_valid_directive = false;
    //     for (size_t i = 0; i < directive_keywords.size() && it != end_it; i++)
    //         found_valid_directive |= set_directive_field(directive_keywords[i], field_dsts[i], it, end_it);
    //     return found_valid_directive;
    // }
    string_vec_it_t find_closing_bracket_it(string_vec_it_t it, string_vec_it_t& end_it)
    {
        int sub_context_lvl = 0;
        while (it != end_it)
        {
            if (*it == "{")
                sub_context_lvl++;
            if (*it == "}" && --sub_context_lvl == 0)
                return it;
            it++;
        }
        std::cerr << "sub lvl = " << sub_context_lvl << std::endl;
        throw runtime_error("Openning bracket never closed.");
    }
}
#endif