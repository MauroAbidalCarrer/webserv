#ifndef HTTP_Message_HPP
# define HTTP_Message_HPP
# include <iostream>
# include <string>
# include <cstdlib>

# include "parsing.hpp"
# include "typedefs.hpp"
# include "sys_calls_warp_arounds.hpp"
// # include "WSexception.hpp"

#define FIRST_READ_BUFFER_SIZE 10

//sys call functions declarations to avoid circular dependencies (-_-)
std::string ws_recv(int socket_fd, int buffer_size, int flags, size_t* nb_read_bytes_ptr);
std::string ws_read(int fd, int buffer_size, size_t* nb_read_bytes_ptr);
std::string ws_recv(int socket_fd, int buffer_size, int flags);
std::string ws_read(int fd, int buffer_size);
# define READ_BUFFER_SIZE 3000
# define RECV_FLAGS 0

class HTTP_Message
{
    private:
    int read_fd;
    int recv_flags;
    string construct_buffer;
    bool header_is_constructed;
    protected:
	bool header_is_parsed;
    public:
    vector<string> first_line;
    parsing::tokenized_text_t header;
    string body;
	bool is_fully_constructed;

    public:
    //constructors and destructors
    HTTP_Message() :
    read_fd(-1), recv_flags(0), header_is_constructed(false), header_is_parsed(false), first_line(), header(), body(), is_fully_constructed(false)
    { }
    protected:
	void partial_constructor(int read_fd, int recv_flags = 0)
	{
        size_t nb_read_bytes = 0;
        if (recv_flags != 0)
            construct_buffer += ws_recv(read_fd, READ_BUFFER_SIZE, RECV_FLAGS, &nb_read_bytes);
        else
            construct_buffer += ws_read(read_fd, READ_BUFFER_SIZE, &nb_read_bytes);
        if (nb_read_bytes == 0)
            throw NoBytesToReadException();
        if (header_is_constructed)
        {
            body.append(construct_buffer);
            construct_buffer.clear();
        }
        //If nb_read_bytes < READ_BUFFER_SIZE we might have read the full message
        if (nb_read_bytes < READ_BUFFER_SIZE)
        {
            cout << "nb_read_bytes: " << nb_read_bytes << ", READ_BUFFER_SIZE: " << READ_BUFFER_SIZE << endl;
            //If nb_read_bytes < READ_BUFFER_SIZE we are (almost)certain that we have read the whole header and
            if (header_is_constructed == false)
            {
                cout << YELLOW_AINSI << "Construct buffer before constructing header:" << endl;
                cout << construct_buffer << END_AINSI << endl;
                construct_header();
                body = construct_buffer.substr(construct_buffer.find("\r\n\r\n"));
                construct_buffer.clear();
            }
            try
            {
                cout << BLUE_AINSI << "header size = " << header.size() << endl;
                for (size_t i = 0; i < header.size(); i++)
                {
                    for (size_t j = 0; j < header[i].size(); j++)
                        cout << header[i][j] << " ";
                    cout << endl;
                }
                cout << body << endl;
                cout << END_AINSI << endl;
                
                //if body size == Content-Length 
                //We've read the entirety of the message.
                vector<string> Content_length_header_field = get_header_fields("Content-Length");
                size_t content_length = std::atoi(Content_length_header_field[1].c_str());
                cout << "Content-Length: " << content_length << endl;
                if (body.length() >= content_length)
                    is_fully_constructed = true;
            }
            // catch(const std::e& e)
            catch(const NoHeaderFieldFoundException& e)
            {
                //No Content-Length was header was provided, therefore there is no body
                PRINT_ERROR("No Content-Length header found.");
                is_fully_constructed = true;
                // throw WSexception("400");
                // throw NoHeaderFieldFoundException();
            }
        }
	}
    private:
    void construct_header()
    {
        parsing::tokenized_text_t tokenized_msg = parsing::tokenize_HTTP_message(construct_buffer);
        first_line = tokenized_msg[0];
        parsing::tokenized_text_t::iterator header_end_it = tokenized_msg.begin() + 1;
        while (header_end_it != tokenized_msg.end() && !header_end_it->empty())
            header_end_it++;
        header = parsing::tokenized_text_t(tokenized_msg.begin() + 1, header_end_it);
        header_is_constructed = true;
    }
    public:
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
    virtual string debug()
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
            vector<string> content_type = get_header_fields("Content-Type");
            if (content_type.size() >= 2 && content_type[1].find("text") != std::string::npos)
            {
                str.append(body);
                str.append(parsing::CLRF);
            }
            else
                str.append("HTTP message body was ommited because body is not text.\n");
        }
        catch(NoHeaderFieldFoundException e) 
        {  
            // std::cout << "Could not found Content-Type header field while debugging response." << std::endl;
        }        
        return str;
    }
    protected:
    vector<string> get_header_fields(std::string header_name)
    {
        for (size_t i = 0; i < header.size(); i++)
            if (header[i].size() > 1 && header[i][0] == header_name)
                return header[i];
        throw NoHeaderFieldFoundException();
    }
    void set_header_fields(std::string header_str, std::string value)
    {
        vector<string> header_fields;
        header_fields.push_back(header_str);
        header_fields.push_back(value);
        for (parsing::tokenized_text_t::iterator i = header.begin(); i != header.end(); i++)
        {
            if (i->size() > 0 && (*i)[0] == header_str)
            {
                *i = header_fields;
                return ;
            }
        }
        header.push_back(header_fields);
    }
    void set_header_fields(vector<string> header_fields)
    {
        if (header_fields.size() == 0)
        {
            cout << YELLOW_WARNING << "HTTP_Message::set_header_fields called with empty header_fields." << endl;
            return;
        }
        for (parsing::tokenized_text_t::iterator i = header.begin(); i != header.end(); i++)
        {
            if (i->size() > 0 && (*i)[0] == header_fields[0])
            {
                *i = header_fields;
                return ;
            }
        }
        header.push_back(header_fields);
    }
    public:
    virtual void clear()
    {
        first_line.clear();
        header.clear();
        body.clear();
    }

    //nested classes
    class NoBytesToReadException : public std::exception
    {
        const char * what() const throw()
        {
            return "NoBytesToReadException";
        }
    };
    class NoHeaderFieldFoundException : public std::exception
    {
        const char * what() const throw()
        {
            return "NoHeaderFieldFoundException";
        }
    };
};
#endif