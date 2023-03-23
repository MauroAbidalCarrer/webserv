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
    std::vector<HTTP_Message>& messages_storage;

    public:

    //constructors and destructors
    HTTP_Message(int read_fd, std::vector<HTTP_Message>& messages_storage) :
    read_fd(read_fd), messages_storage(messages_storage)
    {
        messages_storage.push_back(*this);
    }
    HTTP_Message(const HTTP_Message& other) : messages_storage(other.messages_storage)
    {
        *this = other;
    }
    ~HTTP_Message()
    {
    }
    //operator overloads
    HTTP_Message& operator=(const HTTP_Message& rhs)
    {
        (void)rhs;
        return *this;
    }
};
#endif