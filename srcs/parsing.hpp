#ifndef PARSING
# define PARSING
# include <string>
# include <vector>
# include <sstream>
# include <iostream>

namespace parsing
{

    typedef std::vector<std::string> line_of_tokens_t;
    line_of_tokens_t tokenize(std::string str, std::string delimiter, bool include_empty_tokens)
    {
        std::vector<std::string> tokens;
        size_t pos = 0;
        while (pos != str.length())
        {
            size_t next_pos = str.find(delimiter, pos);
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

    typedef std::vector<line_of_tokens_t> tokenized_file_t;
    tokenized_file_t tokenize_file(std::string file_str, std::string line_delimiter, std::string word_delimiters, bool include_empty_lines)
    {
        tokenized_file_t tokenized_file;
        line_of_tokens_t line_tokens = tokenize(file_str, line_delimiter, include_empty_lines);
        for (size_t i = 0; i < line_tokens.size(); i++)
            tokenized_file.push_back(tokenize(line_tokens[i], word_delimiters, false));
        return tokenized_file;
    }


    const char * CLRF = "\r\n";
    typedef tokenized_file_t tokenized_HTTP_message_t;
    tokenized_HTTP_message_t tokenize_HTTP_message(std::string message_text)
    {
        return tokenize_file(message_text, CLRF, ": ", true);
    }
}

#endif