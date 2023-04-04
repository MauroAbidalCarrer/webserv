#ifndef HTTP_Message_HPP
# define HTTP_Message_HPP
# include <iostream>
# include <string>

# include "parsing.hpp"
# include "sys_calls_warp_arounds.hpp"

#define FIRST_READ_BUFFER_SIZE 1000

//sys call functions declarations to avoid circular dependencies (-_-)
std::string ws_read(int fd, int buffer_size);
std::string ws_recv(int fd, int buffer_size, int recv_flags);


class HTTP_Message
{
    private:
    int read_fd;
    int recv_flags;
    public:
    parsing::line_of_tokens_t first_line;
    parsing::tokenized_text_t header; 
    std::string body; 

    public:
    //constructors and destructors
    HTTP_Message() : recv_flags(0) { }
    HTTP_Message(int read_fd, size_t buffer_size, int recv_flags = 0) : read_fd(read_fd), recv_flags(recv_flags)
    {
        std::string msg_as_text = read_text_msg(buffer_size);
        std::cout << "\"" << msg_as_text << "\"" << std::endl;
        if (msg_as_text.length() == 0)
            throw NoBytesToReadException();
        parsing::tokenized_text_t tokenized_msg = parsing::tokenize_HTTP_message(msg_as_text);
        // if (!msg_as_text.find("\r\n\\n"))
            // throw bad request
        //do parsing  checks
        first_line = tokenized_msg[0];
        parsing::tokenized_text_t::iterator it = tokenized_msg.begin() + 1;
        while (it != tokenized_msg.end() && !it->empty())
            it++;
        //do parsing checks
        header = parsing::tokenized_text_t(tokenized_msg.begin() + 1, it);
        body = msg_as_text.substr(msg_as_text.find("\r\n\r\n"));
    }
    HTTP_Message(const HTTP_Message& other)
    {
        *this = other;
    }
    ~HTTP_Message() { }
    //operator overloads
    HTTP_Message& operator=(const HTTP_Message& rhs)
    {
        read_fd = rhs.read_fd;
        recv_flags = rhs.recv_flags;
        first_line = rhs.first_line;
        header = rhs.header; 
        body = rhs.body; 
        return *this;
    }
    //methods
    std::string read_text_msg(size_t buffer_size)
    {
        if (recv_flags == 0)
            return ws_read(read_fd, buffer_size);
        else
            return ws_recv(read_fd, buffer_size, recv_flags);
    }
    std::string serialize()
    {
        std::string str;
        for (size_t i = 0; i < first_line.size(); i++)
        {
            str.append(first_line[i]);
            str.append(" ");
        }
        str.append(parsing::CLRF);
        for (size_t i = 0; i < header.size(); i++)
        {
            str.append(header[i][0]);
            str.append(":");
            for (size_t j = 1; j < header[i].size(); j++)
            {
                str.append(" ");
                str.append(header[i][j]);
            }
            str.append(parsing::CLRF);
        }
        str.append(parsing::CLRF);
        str.append(body);
        str.append(parsing::CLRF);
        return str;
    }
    std::string debug()
    {
        std::string str;
        for (size_t i = 0; i < first_line.size(); i++)
        {
            str.append(first_line[i]);
            str.append(" ");
        }
        str.append(parsing::CLRF);
        for (size_t i = 0; i < header.size(); i++)
        {
            str.append(header[i][0]);
            str.append(":");
            for (size_t j = 1; j < header[i].size(); j++)
            {
                str.append(" ");
                str.append(header[i][j]);
            }
            str.append(parsing::CLRF);
        }
        str.append(parsing::CLRF);
        try
        {
            parsing::line_of_tokens_t content_type = get_header_fields("Content-Type");
            if (content_type.size() >= 2 && content_type[1].find("text") != std::string::npos)
                str.append(body);
            else
                str.append("HTTP message body was ommited because it is not text.");
        }
        catch(NoHeaderFieldFoundException e) 
        {  
            std::cout << "Could not found Content-Type header field while debugging response." << std::endl;
        }
        str.append(parsing::CLRF);        
        return str;
    }
    parsing::line_of_tokens_t get_header_fields(std::string header_name)
    {
        for (size_t i = 0; i < header.size(); i++)
            if (header[i].size() > 1 && header[i][0] == header_name)
                return header[i];
        throw NoHeaderFieldFoundException();
        // throw std::runtime_error("Header field " + header_name + " not found!");

    }
    void clear()
    {
        first_line.clear();
        header.clear();
        body.clear();
    }

    //nested classes
    class NoBytesToReadException : std::exception
    {
        const char * what() const throw()
        {
            return "NoBytesToReadException";
        }
    };
    class NoHeaderFieldFoundException : std::exception
    {
        const char * what() const throw()
        {
            return "NoHeaderFieldFoundException";
        }
    };
};
#endif