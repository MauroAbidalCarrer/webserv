#ifndef HTTP_Request_HPP
# define HTTP_Request_HPP

# include <iostream>
# include <string>
# include "HTTP_Message.hpp"
# include "parsing.hpp"

# define HOST 1
# define PORT 2

class HTTP_Request : public HTTP_Message
{
	private:
	//fields
	vector<string> request_line;
	public:
	std::string HTTP_method;
	std::string target_URL;

	std::string	_path;
	std::string	_ports;
	std::string	_hostname;
	std::string	_queryString;

	public:
	HTTP_Request() { }
	HTTP_Request(int read_fd, size_t buffer_size, int recv_flags = 0) : HTTP_Message(read_fd, buffer_size, recv_flags)
	{
		//do checks to make sure that the message is a properly formatted request
		request_line = first_line;
		HTTP_method = request_line[0];
		target_URL = request_line[1];
		std::size_t	d = target_URL.find("?");
		this->_path = target_URL.substr(0, d);
		if (d != std::string::npos)
			this->_queryString = target_URL.substr(d + 1, target_URL.size() - d);
		this->_hostname = this->get_header_fields("Host")[HOST];
		if (this->get_header_fields("Host").size() > 2)
			this->_ports = this->get_header_fields("Host")[PORT];
	}

	void	printContent()	{
		std::cout << "[++++++++++++++++ V*A*L*U*E*S +++++++++++++++++++]" << std::endl;
		std::cout << "HTPP_Serveur: Port: " << this->_ports << std::endl;
		std::cout << "HTPP_Serveur: Path: " << this->_path << std::endl;
		std::cout << "HTPP_Serveur: Hostname: " << this->_hostname << std::endl;
		std::cout << "HTPP_Serveur: QueryString: " << this->_queryString << std::endl;
		std::cout << "[++++++++++++++++++++++++++++++++++++++++++++++++]" << std::endl;
	}
};

#endif