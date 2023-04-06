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

# define MAXIMUM_HTTP_HEADER_SIZE 1024
# define GET_RESPONSE_CONTENT_RAD_BUFFER_SIZE 10000

# define CONNEXION_TIMEOUT_MODE no_timeout
# define IDLE_CLIENT_CONNEXION_TIMEOUT_IN_MILL 3000

# define WEB_RESSOURCES_DIRECTORY "web_ressources"
# define DEFAULT_RESSOURCES_DIRECTORY "./web_ressources/default_pages/"

class ClientConnexionHandler : public IO_Manager::FD_interest
{
	private:
	HTTP_Request request;
	HTTP_Response response;
	
	public:
	//constructors and destructors
	ClientConnexionHandler() {}
	ClientConnexionHandler(int socket_connexion_fd) :
	FD_interest(IDLE_CLIENT_CONNEXION_TIMEOUT_IN_MILL, CONNEXION_TIMEOUT_MODE, socket_connexion_fd)
	{ }
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
			
			find_and_apply_contet_to_request();
			/*if request requires CGI generate response
				fork, excve CGI, AND input request to CGI input AND read from CGI output, complete header fields AND send response
			*/
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
	void  find_and_apply_contet_to_request()
	{
		// VirtualServerContext request_virtualServerContext = GlobalContextSingleton.virtual_server_contexts
		//apply context
		//apply root directive(for now just insert ".")
		request.target_URL = std::string(WEB_RESSOURCES_DIRECTORY) + request.target_URL;
		//apply rewrite directive(not sure if it's the rewrite directive... the that completes target URLs finishing ini "/")(for no just index.html)
		if (*(request.target_URL.end() - 1) == '/')
			request.target_URL.append("index.html");
	}
	//GET method
	//open content file AND consturct response from content AND dedebug reuqest on connexion socket_fd
	void start_processing_Get_request()
	{
		response = HTTP_Response::mk_from_file_and_status_code("200", request.target_URL);
		//Implement response method that defines response's Content-Type header field.
		IO_Manager::change_interest_epoll_mask(fd, EPOLLOUT);
	}

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