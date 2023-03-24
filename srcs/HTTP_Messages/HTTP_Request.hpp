#ifndef HTTP_Request_HPP
# define HTTP_Request_HPP
# include <iostream>
# include <string>

# include "HTTP_Message.hpp"

class HTTP_Request : public HTTP_Message
{
    public:
    //constructors and destructors
    HTTP_Request()
    {
        
    }
    HTTP_Request(int read_fd, char *already_read_text) : HTTP_Message(read_fd, already_read_text)
    {
        
    }
    HTTP_Request(const HTTP_Request& other)
    {
        *this = other;
    }
    ~HTTP_Request()
    {
        
    }
    //operator overloads
    HTTP_Request& operator=(const HTTP_Request& rhs)
    {
        (void)rhs;
        return *this;
    }
};
#endif