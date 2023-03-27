#ifndef HTTP_Response_HPP
# define HTTP_Response_HPP
# include <iostream>
# include <string>

# include "HTTP_Message.hpp"

class HTTP_Response : public HTTP_Message
{
    public:
    int status_code;
    std::string status_msg;

    public:
    //constructors and destructors
    HTTP_Response() { }
    HTTP_Response(int read_fd, size_t buffer_size) : HTTP_Message(read_fd, buffer_size)
    {
        //do checks
        status_code = std::atoi(first_line[1].data());
        status_msg = first_line[2];
    }
    HTTP_Response(const HTTP_Response& other)
    {
        *this = other;
    }
    ~HTTP_Response() { }
    //operator overloads
    HTTP_Response& operator=(const HTTP_Response& rhs)
    {
        (void)rhs;
        return *this;
    }
    //methods
    void set_header_fields(std::string header_str, std::string value)
    {
        parsing::line_of_tokens_t header_fields;
        header_fields.push_back(header_str);
        header_fields.push_back(value);
        for (parsing::tokenized_text_t::iterator i = header.begin(); i != header.end(); i++)
        {
            if ((*i)[0] == header_str)
            {
                *i = header_fields;
                return ;
            }
        }
        header.push_back(header_fields);
    }
};
#endif