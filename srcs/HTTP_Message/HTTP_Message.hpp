#ifndef HTTP_Message_HPP
# define HTTP_Message_HPP
# include <iostream>
# include <string>
# include <cstdlib>
# include <algorithm>
# include <iomanip>
# include <climits>

# include "parsing.hpp"
# include "typedefs.hpp"
# include "sys_calls_warp_arounds.hpp"
# include "WSexception.hpp"

// sys call functions declarations to avoid circular dependencies (-_-)
std::string ws_read(int fd, size_t buffer_size, ssize_t *nb_read_bytes_ptr);
// std::string ws_read(int fd, size_t buffer_size);
void throw_WSexcetpion(const string& status_code, const string& what_msg);
void throw_WSexcetpion(const string& status_code);
# define READ_BUFFER_SIZE 100000
# define MAX_HEADER_SIZE 3000

class HTTP_Message
{
private:
    // size_t nb_partial_constructs, total_nb_bytes_read;
    int read_fd;
    // string tmp_header_as_string;
protected:
    string construct_buffer;

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
    read_fd(-1),
    content_length(ULLONG_MAX),
    header_is_constructed(false),
    first_line(),
    header(),
    body(),
    is_fully_constructed(false)
    { }

protected:
    ssize_t construct_header(int read_fd, const string& header_too_long_status_code, const string& bad_header_status_code)
    {
        ssize_t nb_read_bytes = 0;
        construct_buffer += ws_read(read_fd, READ_BUFFER_SIZE, &nb_read_bytes);
        size_t double_CRLF_index = construct_buffer.find("\r\n\r\n");
        if (double_CRLF_index == string::npos)
        {
            if (construct_buffer.size() > MAX_HEADER_SIZE)
                throw_WSexcetpion(header_too_long_status_code);
            return nb_read_bytes;
        }
        string header_as_string = construct_buffer.substr(0, double_CRLF_index);
        parsing::tokenized_HTTP_t tokenized_header = parsing::tokenize_HTTP_message(header_as_string);
        if (tokenized_header.size() == 0 || tokenized_header[0].size() != 3)
            throw_WSexcetpion(bad_header_status_code);
        first_line = tokenized_header[0];
        header = parsing::tokenized_HTTP_t(tokenized_header.begin() + 1, tokenized_header.end());
        header_is_constructed = true;
        return nb_read_bytes;
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
    bool try_get_header_fields(string header_name, vector<string>& vector_dst)
    {
        for (size_t i = 0; i < header.size(); i++)
        {
            if (header[i].size() > 1 && header[i][0] == header_name)
            {
                vector_dst = header[i];
                return true;
            }
        }
        return false;
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
            PRINT_WARNING("HTTP_Message::set_header_fields called with empty header_fields.");
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
    size_t get_content_length_from_header()
    {
        vector<string> content_length_header = get_header_fields("Content-Length");
        if (content_length_header.size() < 2)
            throw_WSexcetpion("400", "Header \"Content-Length\" has no value!");
        return std::strtoul(content_length_header[1].c_str(), NULL, 0);
    }
    void print_body_in_hexa()
    {
        for (size_t i = 0; i < body.length(); i++)
            cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(body[i]) << " ";
        cout << endl;
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
};
#endif