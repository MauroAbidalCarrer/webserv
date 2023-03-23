#ifndef HTTP_Message_HPP
# define HTTP_Message_HPP
# include <iostream>
# include <string>
# include <vector>
# include <algorithm>


class HTTP_Message
{


    public:

    //constructors and destructors
    HTTP_Message()
    {
        
    }
    HTTP_Message(const HTTP_Message& other)
    {
        *this = other;
    }
    ~HTTP_Message()
    {
        // messages_iterator_t it = find(messages.begin(), messages.end(), *this);
        // messages.erase(it);
    }
    //operator overloads
    HTTP_Message& operator=(const HTTP_Message& rhs)
    {
        (void)rhs;
        return *this;
    }
};
#endif