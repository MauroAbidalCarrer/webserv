#ifndef HTTP_Message_HPP
# define HTTP_Message_HPP
# include <iostream>
# include <string>
# include <vector>
# include <algorithm>


class HTTP_Message
{
    protected:
    int read_fd;
    std::string message_as_text;
    //desrialized header fields
    std::string host_name;
    int host_port;
    enum connection_type { keep_alive, close };
    connection_type connection;
    enum content_type_type { text };
    content_type_type content_type;//classic OOP shit


    public:

    //constructors and destructors
    HTTP_Message() { }
    HTTP_Message(int read_fd, char * already_read_text) :
    read_fd(read_fd), message_as_text(already_read_text)
    {
        std::cout << "HTTP request:" << std::endl << message_as_text << std::endl;
    }
    HTTP_Message(const HTTP_Message& other)
    {
        *this = other;
    }
    ~HTTP_Message() { }
    //operator overloads
    HTTP_Message& operator=(const HTTP_Message& rhs)
    {
        (void)rhs;
        return *this;
    }
};
#endif