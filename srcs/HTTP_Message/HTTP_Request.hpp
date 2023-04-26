#ifndef HTTP_Request_HPP
# define HTTP_Request_HPP

# include <iostream>
# include <string>
# include "HTTP_Message.hpp"
# include "parsing.hpp"
# include "LocationContext.hpp"

# define HOST 1
# define PORT 2
# define RECV_FLAGS 0


class HTTP_Request : public HTTP_Message
{
	private:
	//fields
	enum body_type_t { no_body, chunked_body, fixed_size_body };
	//chunk body fields
	body_type_t body_type;
	size_t chunk_begin_i;
	//request fields
	vector<string> request_line;
	public:
	string HTTP_method;
	string target_URL;
	std::string	_path;
	std::string	_ports;
	std::string	_hostname;

	public:
	std::string	_queryString;
	bool is_redirected;

	public:
	HTTP_Request() :
	HTTP_Message(),
	body_type(no_body),
	chunk_begin_i(0),
	request_line(),
	HTTP_method(),
	target_URL(),
	_path(),
	_ports(),
	_hostname(),
	_queryString(),
	is_redirected(false)
	{ }
	//construct redirected GET request
	HTTP_Request(string redirected_path, HTTP_Request initial_request) : 
	body_type(no_body),
	chunk_begin_i(0),
	HTTP_method("GET"),
	_path(redirected_path),
	is_redirected(true)
	{
		first_line.push_back("GET");
		first_line.push_back(_path);
		first_line.push_back("HTTP/1.1");
		try
		{
			vector<string> initial_host_header_fields = initial_request.get_header_fields("Host");
			set_header_fields(initial_host_header_fields);
			if (initial_host_header_fields.size() > 12)
				_hostname = initial_host_header_fields[1];
			if (initial_host_header_fields.size() > 2)
				_ports = initial_host_header_fields[2];
		}
		catch(const NoHeaderFieldFoundException& e)
		{
			PRINT_WARNING("no \"Host\" field found in initial_request while constructing redirected request.");
		}
	}
	void construct_from_socket(int socket_fd)
	{
		if (header_is_constructed == false)
		{
			construct_header(socket_fd);
			// PRINT("header_is_constructed == false: " << header_is_constructed << ", Constuct buffer:" << endl << FAINT_AINSI << construct_buffer << END_AINSI);
		}
		else
		{
			if (body_type == fixed_size_body)
				construct_body_with_fixed_size(socket_fd);
			if (body_type == chunked_body)
			{
				body += ws_read(socket_fd, READ_BUFFER_SIZE);
				// PRINT("current chunk after read:" << endl << FAINT_AINSI << (body.c_str() + chunk_begin_i) << END_AINSI);
				handle_new_chunked_body_content();
			}
		}
	}
	void construct_header(int socket_fd)
	{
		if (HTTP_Message::construct_header(socket_fd, "431", "400") == 0)
			throw NoBytesToReadException();
		if (header_is_constructed)
		{
			request_line = first_line;
			if (request_line[2] != "HTTP/1.1")
				throw_WSexcetpion("505");
			HTTP_method = request_line[0];
			target_URL = request_line[1];
			std::size_t	d = target_URL.find("?");
			this->_path = target_URL.substr(0, d);
			if (d != std::string::npos)
			this->_queryString = target_URL.substr(d + 1, target_URL.size() - d);
			vector<string> host_header_fields;
			//parse Host header field
			if (try_get_header_fields("Host", host_header_fields) == false)
				throw_WSexcetpion("400", "No header field \"Host\" found!");
			if (host_header_fields.size() < 1)
				throw_WSexcetpion("400", "\"Host\" header has no value!");
			this->_hostname = this->get_header_fields("Host")[HOST];
			if (this->get_header_fields("Host").size() > 2)
				this->_ports = this->get_header_fields("Host")[PORT];
			//parse Content-Length header
			vector<string> content_length_header_fields ;
			if (try_get_header_fields("Content-Length", content_length_header_fields))
			{
				content_length = get_content_length_from_header();
				if (content_length > 300000000)
					throw_WSexcetpion("413", "Content-Length is too long.");
				PRINT("Request has fixed size body, content_length: " << content_length);
				body.reserve(content_length);
				body.insert(body.begin(), construct_buffer.begin() + construct_buffer.find("\r\n\r\n") + 4, construct_buffer.end());
				is_fully_constructed = body.length() >= content_length;
				body_type = fixed_size_body;
			}
			else
				content_length = 0;
			vector<string> type_encoding_header_fiels;
			if (try_get_header_fields("Transfer-Encoding", type_encoding_header_fiels))
			{
				if (type_encoding_header_fiels.size() != 2)
					throw_WSexcetpion("400", "Transfer-Encoding header field has no value.");
				if (type_encoding_header_fiels[1] != "chunked")
					throw_WSexcetpion("501", "Transfer-Encoding " + type_encoding_header_fiels[1] + " is not supported!");
				body_type = chunked_body;
				body.insert(body.begin(), construct_buffer.begin() + construct_buffer.find("\r\n\r\n") + 4, construct_buffer.end());
				PRINT("Header:" << endl << FAINT_AINSI << get_header_as_string() << END_AINSI);
				// PRINT("Construct header:" << endl << construct_buffer);
				// PRINT("Body:"  << FAINT_AINSI << END_AINSI << endl << body);
				handle_new_chunked_body_content();
			}
			if (type_encoding_header_fiels.empty() == false && content_length_header_fields.empty() == false)
				throw_WSexcetpion("400", "Both \"Transfer-Encoding\" and \"Content-Length\" headers are present!");
			if (type_encoding_header_fiels.empty() == true && content_length_header_fields.empty() == true)
			{
# ifndef NO_DEBUG
				PRINT_FAINT("Neither \"Content-Length\" and \"Transfer-Encoding\" header fields were found, assuming that the request has no body.");
# endif
				is_fully_constructed = true;
			}
		}
	}	
    void construct_body_with_fixed_size(int read_fd)
    {
		ssize_t nb_readbytes = -1;
		// size_t prev_body_size = body.size();
		body += ws_read(read_fd, READ_BUFFER_SIZE, &nb_readbytes);
		// PRINT("Read body.size() after body read: " << body.size() << ", prev_body_size: " << prev_body_size << ", nb_read_bytes:" << nb_readbytes);
        if (nb_readbytes == -1)
            throw runtime_error("Could not read on fd to cosntruct HTTP message.");
		is_fully_constructed = body.length() >= content_length;
        if (nb_readbytes == 0)
            throw NoBytesToReadException();
    }
	void handle_new_chunked_body_content()
	{
		size_t chunk_size;
		do
		{
			//check that there actually is a chunk header.
			size_t chunk_header_end_i = get_next_CRLF_index();
			if (chunk_header_end_i == string::npos)
			{
				PRINT("No chunk header found, body:" << endl << body.c_str() + chunk_begin_i);
				return ;
			}
			//get chunk size in chunk header
			chunk_size = std::strtoul(body.c_str() + chunk_begin_i, NULL, 16);
			bool full_chunk_is_in_body = body.length() - chunk_begin_i >= chunk_size + 2 + chunk_header_end_i;
			if (full_chunk_is_in_body == false)
				break;
			//erase chunk header debugging
			//erase chunk header
			body.erase(chunk_begin_i, get_next_CRLF_index() + 2);
			//skip chunk content
			chunk_begin_i += chunk_size;
			//erase chunk terminating CRLF
			body.erase(chunk_begin_i, 2);
		}
		while (chunk_size != 0);
		is_fully_constructed = chunk_size == 0;
		if (is_fully_constructed)
		{
			content_length = body.length();
			PRINT("Chunked request fully constructed.");
		}
		// PRINT("is_fully_constructed: " << is_fully_constructed << endl);
	}
	size_t get_next_CRLF_index()
	{
		for (size_t i = 0; chunk_begin_i + i + 1 < body.size(); i++)
		{
			if (body[chunk_begin_i + i] == '\r' && body[chunk_begin_i + i + 1] == '\n')
				return i;
		}
		return string::npos;
	}
	void	printContent()	
	{
		PRINT("[++++++++++++++++ V*A*L*U*E*S +++++++++++++++++++]");
		PRINT("HTPP_Serveur: Port: " << this->_ports);
		PRINT("HTPP_Serveur: Path: " << this->_path);
		PRINT("HTPP_Serveur: Hostname: " << this->_hostname);
		PRINT("HTPP_Serveur: QueryString: " << this->_queryString);
		PRINT("[++++++++++++++++++++++++++++++++++++++++++++++++]");
	}
};
#endif