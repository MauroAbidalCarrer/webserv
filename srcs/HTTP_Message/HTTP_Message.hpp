#ifndef HTTP_Message_HPP
#define HTTP_Message_HPP
#include <iostream>
#include <string>
#include <cstdlib>
#include <algorithm>

#include "parsing.hpp"
#include "typedefs.hpp"
#include "sys_calls_warp_arounds.hpp"
// # include "WSexception.hpp"

// sys call functions declarations to avoid circular dependencies (-_-)
std::string ws_recv(int socket_fd, size_t buffer_size, int flags, ssize_t *nb_read_bytes_ptr);
std::string ws_read(int fd, size_t buffer_size, ssize_t *nb_read_bytes_ptr);
std::string ws_recv(int socket_fd, size_t buffer_size, int flags);
std::string ws_read(int fd, size_t buffer_size);
#define READ_BUFFER_SIZE 10000
#define RECV_FLAGS 0

class HTTP_Message
{
private:
    // size_t nb_partial_constructs, total_nb_bytes_read;
    int read_fd;
    int recv_flags;
    string construct_buffer;
    // string tmp_header_as_string;

public:
    size_t content_length;
    bool header_is_constructed;
    vector<string> first_line;
    parsing::tokenized_text_t header;
    string body;
    bool is_fully_constructed;

public:
    // constructors and destructors
    HTTP_Message() : 
    // nb_partial_constructs(0),
    // total_nb_bytes_read(0),
    read_fd(-1),
    recv_flags(0),
    // tmp_header_as_string(),
    content_length(0),
    header_is_constructed(false),
    first_line(),
    header(),
    body(),
    is_fully_constructed(false)
    { }

protected:
    void partial_constructor(int read_fd)
    {
        if (header_is_constructed == false)
        {
            ssize_t nb_read_bytes = 0;
            construct_buffer += ws_read(read_fd, READ_BUFFER_SIZE, &nb_read_bytes);
            if (nb_read_bytes == 0)
                throw NoBytesToReadException();
            size_t double_CRLF_index = construct_buffer.find("\r\n\r\n");
            if (double_CRLF_index == string::npos)
                return ;
            string header_as_string = construct_buffer.substr(0, double_CRLF_index);
            parsing::tokenized_HTTP_t tokenized_header = parsing::tokenize_HTTP_message(header_as_string);
            first_line = tokenized_header[0];
            header = parsing::tokenized_HTTP_t(tokenized_header.begin() + 1, tokenized_header.end());
            try
            {
                // vector<string> content_length_header = get_header_fields("Content-Length");
                content_length =  get_content_length_from_header();//std::strtoul(content_length_header[1].c_str(), NULL, 0);
                body.reserve(content_length);
                body.insert(body.begin(), construct_buffer.begin() + double_CRLF_index + 4, construct_buffer.end());
            }
            catch(const std::exception& e)
            {
                // cout << FAINT_AINSI << "No header \"Content-Length\"(match is case sensitive) was found, assuming that there is no body, (Could be chunked, not yet supported)" << END_AINSI << endl;
                is_fully_constructed = true;
            }
            header_is_constructed = true;
        }
        else
        {
            size_t read_size = content_length - body.size() < READ_BUFFER_SIZE ? content_length - body.size() : READ_BUFFER_SIZE;
            size_t prev_body_size = body.size();
            ssize_t nb_readytes = read(read_fd, (void *)(body.data() + prev_body_size), read_size);
            if (nb_readytes == -1)
                throw runtime_error("Could not read on fd to cosntruct HTTP message.");
            if (nb_readytes == 0)
                throw NoBytesToReadException();
            body.resize(body.size() + nb_readytes);
            // double percentage_of_body_bytes_read = (double)((double)body.length() / (double)content_length) * (double)100.0;
            // cout << "Read bytes into body, body.length(): " << body.length() << ", content_length: " << content_length << ", percentage of bytes read: " << percentage_of_body_bytes_read << endl;
        }
        is_fully_constructed = body.length() >= content_length;
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
    HTTP_Message(const HTTP_Message &other)
    {
        *this = other;
    }
    ~HTTP_Message() {}
    // operator overloads
    HTTP_Message &operator=(const HTTP_Message &rhs)
    {
        read_fd = rhs.read_fd;
        recv_flags = rhs.recv_flags;
        first_line = rhs.first_line;
        header = rhs.header;
        body = rhs.body;
        return *this;
    }
    // methods
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
        catch (NoHeaderFieldFoundException e)
        {
            str.append("Did not found \'Content-Length\' header(match is case sensitive), assuming that there is no body.");
            // std::cout << "Could not found Content-Type header field while debugging response." << std::endl;
        }
        return str;
    }
    vector<string> get_header_fields(std::string header_name)
    {
        for (size_t i = 0; i < header.size(); i++)
            if (header[i].size() > 1 && header[i][0] == header_name)
                return header[i];
        throw NoHeaderFieldFoundException();
    }

protected:
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
                return;
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
                return;
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

    // nested classes
    class NoBytesToReadException : public std::exception
    {
        const char *what() const throw()
        {
            return "NoBytesToReadException";
        }
    };
    class NoHeaderFieldFoundException : public std::exception
    {
        const char *what() const throw()
        {
            return "NoHeaderFieldFoundException";
        }
    };
    size_t get_content_length_from_header()
    {
        vector<string> content_length_header = get_header_fields("Content-Length");
        if (content_length_header.size() < 2)
            throw runtime_error("Header \"Content-Length\" is present but no actual length is specified");
        return std::strtoul(content_length_header[1].c_str(), NULL, 0);
    }
};
#endif