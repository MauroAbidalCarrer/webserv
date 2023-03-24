#ifndef HTTP_RequestHandler_HPP
# define HTTP_RequestHandler_HPP
# include <iostream>
# include <string>
# include <vector>

# include "HTTP_Message.hpp"
# include "HTTP_Request.hpp"

class HTTP_RequestHandler
{
    private:
    std::vector<HTTP_RequestHandler>& handlers_storage;
    typedef std::vector<HTTP_RequestHandler>::iterator storage_it_t;
    HTTP_Request request;

    public:
    //constructors and destructors
    HTTP_RequestHandler(int read_fd, char * already_read_request_text, std::vector<HTTP_RequestHandler>& messages_storage) :
    handlers_storage(messages_storage), request(read_fd, already_read_request_text)
    {
        std::cout << "HTTP_RequestHandler constructor called" << std::endl;
    }
    HTTP_RequestHandler(const HTTP_RequestHandler& other) : handlers_storage(other.handlers_storage), request()
    {
        *this = other;
    }
    ~HTTP_RequestHandler()
    {
        
    }
    //operator overloads
    HTTP_RequestHandler& operator=(const HTTP_RequestHandler& rhs)
    {
        (void)rhs;
        return *this;
    }
};
#endif