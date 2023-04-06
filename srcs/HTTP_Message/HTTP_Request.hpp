#ifndef HTTP_Request_HPP
# define HTTP_Request_HPP

# include <iostream>
# include <string>
# include "HTTP_Message.hpp"
# include "parsing.hpp"

# define HOST 1
# define PORT 2

typedef	std::vector<std::pair<std::string, std::string> >	PRM;

class HTTP_Request : public HTTP_Message
{
	private:
	//fields
	vector<string> request_line;
	public:
	std::string HTTP_method;
	std::string target_URL;

	PRM			_param;
	std::string	_path;
	std::string	_ports;
	std::string	_hostname;

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
			this->URL_PRM(std::string(target_URL));
		this->_hostname = this->get_header_fields("Host")[HOST];
		if (this->get_header_fields("Host").size() > 2)
			this->_ports = this->get_header_fields("Host")[PORT];
		// this->printContent();
	}

	void	URL_PRM(std::string s)	{
		std::string lhs;
		std::string rhs;
		std::size_t	p;
		s.erase(0, 2);
		while (true)	{
			p = s.find("=");
			if (p == std::string::npos)
				break ;
			lhs = s.substr(0, p);
			s.erase(0, p + 1);
			p = s.find("&", 0);
			rhs = s.substr(0, p);
			s.erase(0, p + 1);
			// std::cout << " LHS:[" << lhs << "] " << "RHS:[" << rhs << "]" << std::endl;
			this->_param.push_back(std::make_pair(lhs, rhs));
		}
	}


	void	printContent()	{
		std::cout << "[++++++++++++++++ V*A*L*U*E*S +++++++++++++++++++]" << std::endl;
		std::cout << "HTPP_Serveur: Port: " << this->_ports << std::endl;
		std::cout << "HTPP_Serveur: Path: " << this->_path << std::endl;
		std::cout << "HTPP_Serveur: Hostname: " << this->_hostname << std::endl;
		std::cout << "HTPP_Serveur: PARAM: ";
		for (PRM::iterator it = this->_param.begin(); it != this->_param.end(); it++)
			std::cout << " LHS:[" << it->first << "] " << "RHS:[" << it->second << "]" << std::endl;
		std::cout << "[++++++++++++++++++++++++++++++++++++++++++++++++]" << std::endl;
	}
};
#endif