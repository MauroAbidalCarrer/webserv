#ifndef PARSING
# define PARSING
# include <string>
# include <vector>
# include <sstream>
# include <iostream>
# include <algorithm>

namespace parsing
{
    typedef size_t (std::string::*next_pos_finder_t)(const std::string&, size_t) const;
    typedef std::vector<std::string> line_of_tokens_t;
    line_of_tokens_t tokenize(std::string str, next_pos_finder_t next_pos_finder, std::string delimiter, bool include_empty_tokens)
    {
        std::vector<std::string> tokens;
        size_t pos = 0;
        while (pos != str.length())
        {
            size_t next_pos = ((&str)->*next_pos_finder)(delimiter, pos);
            if (next_pos == std::string::npos)
            {
                tokens.push_back(str.substr(pos));
                break;
            }
            size_t len = next_pos - pos;
            if (len != 0 || include_empty_tokens)
                tokens.push_back(str.substr(pos, len));
            pos = next_pos + delimiter.length();
        }
        return tokens;
    }

    typedef std::vector<line_of_tokens_t> tokenized_text_t;
    tokenized_text_t tokenize_text(std::string file_str, std::string line_delimiter, std::string word_delimiters, bool include_empty_lines)
    {
        tokenized_text_t tokenized_file;
        line_of_tokens_t line_tokens = tokenize(file_str, &std::string::find, line_delimiter, include_empty_lines);
        for (size_t i = 0; i < line_tokens.size(); i++)
            tokenized_file.push_back(tokenize(line_tokens[i], &std::string::find_first_of, word_delimiters, false));
        return tokenized_file;
    }


    const char * CLRF = "\r\n";
    typedef tokenized_text_t tokenized_HTTP_message_t;
    tokenized_HTTP_message_t tokenize_HTTP_message(std::string message_text)
    {
        return tokenize_text(message_text, CLRF, ": ", true);
    }
}

#endif