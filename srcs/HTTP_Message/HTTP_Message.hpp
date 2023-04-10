#ifndef HTTP_Message_HPP
# define HTTP_Message_HPP
# include <iostream>
# include <string>

# include "parsing.hpp"
# include "typedefs.hpp"
# include "sys_calls_warp_arounds.hpp"

#define FIRST_READ_BUFFER_SIZE 1000

//sys call functions declarations to avoid circular dependencies (-_-)
std::string ws_read(int fd, int buffer_size);
std::string ws_recv(int fd, int buffer_size, int recv_flags);

# define READ_BUFFER_SIZE 10
# define RECV_FLAGS 0

class HTTP_Message
{
    private:
	bool header_is_parsed;
    int read_fd;
    int recv_flags;
    public:
    vector<string> first_line;
    parsing::tokenized_text_t header;
    std::string body;
	bool is_completed;

    public:
    //constructors and destructors
    HTTP_Message() { }
    // HTTP_Message(int read_fd, size_t buffer_size, int recv_flags = 0) : read_fd(read_fd), recv_flags(recv_flags)
    // {
    //     std::string msg_as_text = read_text_msg(buffer_size);
    //     if (msg_as_text.length() == 0)
    //         throw NoBytesToReadException();
    //     // cout << "Read new HTTP Message from fd " << read_fd << ":" << endl;
    //     // cout << "\e[2m" << msg_as_text << "\e[0m" << endl;
    //     parsing::tokenized_text_t tokenized_msg = parsing::tokenize_HTTP_message(msg_as_text);
    //     //do parsing checks?
    //     first_line = tokenized_msg[0];
    //     parsing::tokenized_text_t::iterator it = tokenized_msg.begin() + 1;
    //     while (it != tokenized_msg.end() && !it->empty())
    //         it++;
    //     //do parsing checks?
    //     header = parsing::tokenized_text_t(tokenized_msg.begin() + 1, it);
    //     body = msg_as_text.substr(msg_as_text.find("\r\n\r\n"));
    // }

	void mk_request_from_socket(int socket_fd)
	{
		string string_buffer = ws_recv(socket_fd, READ_BUFFER_SIZE, RECV_FLAGS);
		parsing::tokenized_HTTP_message_t tokenized_buffer = parsing::tokenize_HTTP_message(string_buffer);
		parsing::tokenized_HTTP_message_t::iterator it = tokenized_buffer.begin();
        parsing::tokenized_HTTP_message_t::iterator end = tokenized_buffer.end();
		if (!header_is_parsed)
			parse_header(it, end);
		else
            parse_body(it, end);
	}
	void parse_header(parsing::tokenized_HTTP_message_t::iterator& it, parsing::tokenized_HTTP_message_t::iterator end)
	{
		if (first_line.size() != 0)
		{
			first_line = *it;
			it++;
		}
		for (; it != end; it++)
		{
			//arrived ath the end of header
			if (it->size() != 0)
			{
				header_is_parsed = true;
				return;
			}
			header.push_back(*it);
		}
	}
    void parse_body(parsing::tokenized_HTTP_message_t::iterator& it, parsing::tokenized_HTTP_message_t::iterator end)
    {
        
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