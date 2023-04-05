#ifndef HTTP_Request_HPP
# define HTTP_Request_HPP
# include <iostream>
# include <string>

# include "HTTP_Message.hpp"
# include "parsing.hpp"

class HTTP_Request : public HTTP_Message
{
    private:
    //fields
    vector<string> request_line;
    public:
    std::string HTTP_method;
    std::string target_URL;
    
    public:
    HTTP_Request() {};
    HTTP_Request(int read_fd, size_t buffer_size, int recv_flags = 0) : HTTP_Message(read_fd, buffer_size, recv_flags)
    {
        //do checks to make sure that the message is a properly formatted request
        request_line = first_line;
        HTTP_method = request_line[0];
        target_URL = request_line[1];
    }
};
#endif