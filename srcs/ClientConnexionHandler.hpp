#ifndef HTTP_RequestHandler_HPP
# define HTTP_RequestHandler_HPP
# include <iostream>
# include <string>
# include <vector>

# include "IO_Manager.hpp"
# include "HTTP_Message.hpp"
# include "HTTP_Response.hpp"
# include "HTTP_Request.hpp"
# include "Server.hpp"
# include "sys_calls_warp_arounds.hpp"
# include "WSexception.hpp"
# include "typedefs.hpp"
# include "GlobalContext.hpp"
# include "VirtualServerContext.hpp"

# define MAXIMUM_HTTP_HEADER_SIZE 1024
# define GET_RESPONSE_CONTENT_RAD_BUFFER_SIZE 10000

# define CONNEXION_TIMEOUT_MODE no_timeout
# define IDLE_CLIENT_CONNEXION_TIMEOUT_IN_MILL 3000

# define WEB_RESSOURCES_DIRECTORY "web_ressources"
# define DEFAULT_RESSOURCES_DIRECTORY "./web_ressources/default_pages/"

extern GlobalContext GlobalContextSingleton;

class ClientConnexionHandler : public IO_Manager::FD_interest
{
	private:
	HTTP_Request request;
	HTTP_Response response;
	VirtualServerContext virtualServerContext;
	LocationContext locationContext;

	public:
	string listening_ip, listening_port;
	
	public:
	//constructors and destructors
	ClientConnexionHandler() {}
	ClientConnexionHandler(int socket_connexion_fd, string listening_ip, string listening_port) :
	FD_interest(IDLE_CLIENT_CONNEXION_TIMEOUT_IN_MILL, CONNEXION_TIMEOUT_MODE, socket_connexion_fd),
	listening_ip(listening_ip), listening_port(listening_port)
	{ 
		cout << "ClientConnexionHandler.listening_ip = " << listening_ip << ", ClientConnexionHandler.listening_port" << listening_port << endl;
	}
	ClientConnexionHandler(const ClientConnexionHandler& other)
	{
		*this = other;
	}
	~ClientConnexionHandler() {}
	//operator overloads
	ClientConnexionHandler& operator=(const ClientConnexionHandler& rhs)
	{
		(void)rhs;
		return *this;
	}

	//methods
	void handle_client_request()
	{
		try
		{
			request = HTTP_Request(fd, MAXIMUM_HTTP_HEADER_SIZE);
			find_corresponding_contexts();
			apply_context_to_request();
			
			if (request_requires_cgi())
				cout << BLUE_AINSI << "Request requires CGI" << END_AINSI << endl;

			if (request.HTTP_method == "GET")
				start_processing_Get_request();

			//if method == POST
				//if already exists overwrite or append?
				//open target ressource AND write on it and construct response AND dedebug reuqest on connexion socket_fd

			//if method == DELETE
				//if file doesn't exist ?
				//delete file and construct response AND dedebug reuqest on connexion socket_fd
				
			response.set_header_fields("Connection", "Keep-Alive");
		}
		catch (const HTTP_Message::NoBytesToReadException& e)
		{
			std::cout << "Client \e[1mclosed connexion " << fd << "\e[0m." << std::endl;
			close_connexion();
		}
		//make sure to take a reference instead of a copy, otherwise the destructor will be called twice and will potentially call delete twice on the same pointer
		catch(const WSexception& e)
		{
			response = e.response;
			std::cout << "Caught WSexception while processing client request." << std::endl;
			std::cout << "e.what(): " << e.what() << std::endl;
			std::cout << "response: " << std::endl;
			std::cout << response.debug();
			std::cout << "-----------------------" << std::endl;
			IO_Manager::change_interest_epoll_mask(fd, EPOLLOUT);
		}
		catch(const std::exception& e)
		{
			response = HTTP_Response::Mk_default_response("500");
			std::cerr << RED_AINSI << "ERROR" << END_AINSI << ": Unexpected std::exception caught while starting to process request. Setting response to default 500 response. " << std::endl;
			std::cerr << "e.what(): " << e.what() << std::endl;
			IO_Manager::change_interest_epoll_mask(fd, EPOLLOUT);
		}
	}
	//methods
	void find_corresponding_contexts()
	{
		virtualServerContext = GlobalContextSingleton.find_corresponding_virtualServerContext(request, listening_ip, listening_port);
		locationContext = virtualServerContext.find_corresponding_location_context(request);
	}
	void  apply_context_to_request()
	{
		find_corresponding_contexts();
		//apply root directive(for now just insert ".")
		request._path = locationContext.root + request._path;
		//apply rewrite directive(not sure if it's the rewrite directive... the that completes target URLs finishing ini "/")(for no just index.html)
		if (*(request._path.end() - 1) == '/')
			request._path.append(locationContext.default_file);
	}
	//GET method
	//open content file AND consturct response from content AND dedebug reuqest on connexion socket_fd
	void start_processing_Get_request()
	{
		response = HTTP_Response::mk_from_file_and_status_code("200", request._path);
		//Implement response method that defines response's Content-Type header field.
		IO_Manager::change_interest_epoll_mask(fd, EPOLLOUT);
	}
	bool request_requires_cgi()
	{
		for (size_t i = 0; i < locationContext.cgi_extensions_and_launchers.size(); i++)
		{
			pair<string, string> cgi_extension_and_launcher = locationContext.cgi_extensions_and_launchers[i];
			string extension = cgi_extension_and_launcher.first;
			if (str_end_with(request._path, extension))
				return true;
		}
		return false;
	}
	bool str_end_with(string const &fullString, string const &ending)
	{
		if (fullString.length() >= ending.length()) 
			return (0 == fullString.compare (fullString.length() - ending.length(), ending.length(), ending));
		return false;
	}

	// void handle_cgi(string cgi_launcher)
	// {
		
	// }

	void send_response()
	{
		try
		{
			std::string serialized_response = response.serialize();
			std::cout << "sending response:" << std::endl << "\e[2m" << response.debug() << "\e[0m" << "--------------------" << std::endl;
			ws_send(fd, serialized_response, 0);
			response.clear();
			request.clear();
			IO_Manager::change_interest_epoll_mask(fd, EPOLLIN);
		}
		catch(std::exception e)
		{
			std::cerr << "Exception caught in ClientConnexionHandler::" << __func__ << ", client connexion socket fd: " << fd << "." <<std::endl;
			std::cerr << "exception.what() = " << e.what() << std::endl;
			std::cerr << "debuged response: " << response.debug() << "-------------------" << std::endl;
			std::cerr << "\e[1mClosing\e[0m client connexion \e[1m" << fd << "\e[0m." << std::endl;
			close_connexion();
		}
	}

	void internal_call_event_callbacks(epoll_event event)
	{
		if ((event.events & EPOLLIN) && (event.events & EPOLLOUT))
			std::cerr << "Warning: called ClientConnexionHandler::" << __func__ << " with both EPOLLIN and OUT, this is not supposed to happen!" << std::endl;
		if (event.events & EPOLLOUT)
			send_response();
		if (event.events & EPOLLIN)
			handle_client_request();
	}
	void call_timeout_callback()
	{
		std::cerr << "Warning: ClientConnexionHandler::" << __func__ << " called but client is not supposed to have timeout!(not closing connexion)" << std::endl;
	}
	void close_connexion()
	{
		IO_Manager::remove_interest_and_close_fd(fd);
	}
};
#endif