#ifndef HTTP_Request_HPP
# define HTTP_Request_HPP

# include <iostream>
# include <string>
# include "HTTP_Message.hpp"
# include "parsing.hpp"

# define HOST 1
# define PORT 2
# define RECV_FLAGS 0


class HTTP_Request : public HTTP_Message
{
	private:
	//fields
	// string tmp_content;
	vector<string> request_line;
	public:
	string HTTP_method;
	string target_URL;
	std::string	_path;
	std::string	_ports;
	std::string	_hostname;

	public:
	bool is_redirected;
	std::string	_queryString;

	public:
	HTTP_Request() :
	HTTP_Message(), request_line(), HTTP_method(), target_URL(), _path(), _ports(), _hostname(), is_redirected(false), _queryString()
	{ }
	// HTTP_Request(int read_fd) : /* HTTP_Message(read_fd, buffer_size), */ is_redirected(false)
	// {
	// 	//do checks to make sure that the message is a properly formatted request
	// 	request_line = first_line;
	// 	HTTP_method = request_line[0];
	// 	target_URL = request_line[1];
	// 	std::size_t	d = target_URL.find("?");
	// 	this->_path = target_URL.substr(0, d);
	// 	if (d != std::string::npos)
	// 		this->_queryString = target_URL.substr(d + 1, target_URL.size() - d);
	// 	this->_hostname = this->get_header_fields("Host")[HOST];
	// 	if (this->get_header_fields("Host").size() > 2)
	// 		this->_ports = this->get_header_fields("Host")[PORT];
	// }
	//construct redirected GET request
	HTTP_Request(string redirected_path, HTTP_Request initial_request) : 
	HTTP_method("GET"), _path(redirected_path), is_redirected(true)
	{
		first_line.push_back("GET");
		first_line.push_back(_path);
		first_line.push_back("HTTP/1.1");
		try
		{
			vector<string> initial_host_header_fields = initial_request.get_header_fields("Host");
			// cout << "initial_host_header_fields.size() = " << initial_host_header_fields.size() << endl;
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
		// cout << "redirected request:" << endl << debug() << endl;
	}
	void construct_from_socket(int socket_fd)
	{
		if (header_is_constructed == false)
			construct_header(socket_fd);
		else
			construct_body(socket_fd);
	}
	void construct_header(int socket_fd)
	{
		HTTP_Message::construct_header(socket_fd, "431", "400");
		if (header_is_constructed)
		{
			request_line = first_line;
			if (request_line[2] != "HTTP/1.1")
				throw_WSexcetpion("505");
			HTTP_method = request_line[0];
			target_URL = request_line[1];
			std::size_t	d = target_URL.find("?");
			this->_path = target_URL.substr(0, d);
			try
			{
				if (d != std::string::npos)
				this->_queryString = target_URL.substr(d + 1, target_URL.size() - d);
				this->_hostname = this->get_header_fields("Host")[HOST];
				if (this->get_header_fields("Host").size() > 2)
					this->_ports = this->get_header_fields("Host")[PORT];
			}
			catch(NoHeaderFieldFoundException& e)
			{ throw_WSexcetpion("400", "No header field \"Host\" found."); }
			try
			{
				content_length = get_content_length_from_header();
				PRINT("Content_length: " << content_length);
				body.reserve(content_length);
				body.insert(body.begin(), construct_buffer.begin() + construct_buffer.find("\r\n\r\n") + 4, construct_buffer.end());
				is_fully_constructed = body.length() >= content_length;
			}
			catch(const std::exception& e)
			{
				PRINT_FAINT("No header \"Content-Length\"(match is case sensitive) was found while constructing request, assuming that there is no body(Could be chunked, not yet supported).");
				is_fully_constructed = true;
			}
		}
	}	
    void construct_body(int read_fd)
    {
        size_t read_size = content_length - body.size() < READ_BUFFER_SIZE ? content_length - body.size() : READ_BUFFER_SIZE;
        size_t prev_body_size = body.size();
        body.resize(body.size() + read_size);
        ssize_t nb_readytes = read(read_fd, (void *)(body.data() + prev_body_size), read_size);
        if (nb_readytes == -1)
            throw runtime_error("Could not read on fd to cosntruct HTTP message.");
        if (nb_readytes == 0)
            throw NoBytesToReadException();
        body.resize(body.size() - (read_size - nb_readytes));
		is_fully_constructed = body.length() >= content_length;
    }
	// void construct_from_socket(int socket_fd)
	// {
	// 	HTTP_Message::partial_constructor(socket_fd);
	// 	if (is_fully_constructed)
	// 	{
	// 		request_line = first_line;
	// 		HTTP_method = request_line[0];
	// 		target_URL = request_line[1];
	// 		std::size_t	d = target_URL.find("?");
	// 		this->_path = target_URL.substr(0, d);
	// 		try
	// 		{
	// 			if (d != std::string::npos)
	// 			this->_queryString = target_URL.substr(d + 1, target_URL.size() - d);
	// 			this->_hostname = this->get_header_fields("Host")[HOST];
	// 			if (this->get_header_fields("Host").size() > 2)
	// 				this->_ports = this->get_header_fields("Host")[PORT];
	// 		}
	// 		catch(NoHeaderFieldFoundException& e)
	// 		{
	// 			PRINT_ERROR("Caught exception while trying to get \"Host\" header.");
	// 			cout << "request:" << endl << serialize() << endl;
	// 		}
	// 	}
	// }

	void	printContent()	{
		std::cout << "[++++++++++++++++ V*A*L*U*E*S +++++++++++++++++++]" << std::endl;
		std::cout << "HTPP_Serveur: Port: " << this->_ports << std::endl;
		std::cout << "HTPP_Serveur: Path: " << this->_path << std::endl;
		std::cout << "HTPP_Serveur: Hostname: " << this->_hostname << std::endl;
		std::cout << "HTPP_Serveur: QueryString: " << this->_queryString << std::endl;
		std::cout << "[++++++++++++++++++++++++++++++++++++++++++++++++]" << std::endl;
	}
	// void clear()
	// {
	// 	HTTP_Message::clear();
	// 	tmp_content.clear();
	// 	request_line.clear();
	// 	HTTP_method.clear();
	// 	target_URL.clear();
	// 	_path.clear();
	// 	_ports.clear();
	// 	_hostname.clear();
	// 	is_redirected = false;
	// 	std::string	_queryString;
	// 	bool is_fully_constructed;
	// }
};
#endif