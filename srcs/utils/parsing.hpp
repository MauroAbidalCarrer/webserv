#ifndef PARSING 
# define PARSING
# include <string>
# include <vector>
# include <sstream>
# include <iostream>
# include <algorithm>

namespace parsing
{
    typedef std::vector<std::string> line_of_tokens_t;
    line_of_tokens_t tokenize(std::string str, std::string delimiters, bool include_empty_tokens)
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
    line_of_tokens_t tokenize_first_of(std::string str, std::string delimiters, bool include_empty_tokens)
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
    const char * CLRF = "\r\n";

    typedef std::vector<line_of_tokens_t> tokenized_text_t;
    tokenized_text_t tokenize_text(std::string file_str, std::string line_delimiter, std::string word_delimiters)
    {
        tokenized_text_t tokenized_file;
        line_of_tokens_t line_tokens = tokenize(file_str, line_delimiter, true);
        for (size_t i = 0; i < line_tokens.size(); i++)
            tokenized_file.push_back(tokenize_first_of(line_tokens[i], word_delimiters, false));
        return tokenized_file;
    }


    typedef tokenized_text_t tokenized_HTTP_message_t;
    tokenized_HTTP_message_t tokenize_HTTP_message(std::string message_text)
    {
        return tokenize_text(message_text, CLRF, ": ");
    }
}

#endif