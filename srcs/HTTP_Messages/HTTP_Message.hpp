#ifndef HTTP_Message_HPP
# define HTTP_Message_HPP
# include <iostream>
# include <string>
# include <vector>
# include <algorithm>


class HTTP_Message
{
    private:
    int read_fd;
    std::string text_message;
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
    read_fd(read_fd), text_message(already_read_text)
    {
        std::cout << "HTTP request:" << std::endl << text_message << std::endl;
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