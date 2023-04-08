#ifndef HTTP_RequestHandler_HPP
#define HTTP_RequestHandler_HPP
#include <iostream>
#include <string>
#include <vector>

#include "IO_Manager.hpp"
#include "HTTP_Message.hpp"
#include "HTTP_Response.hpp"
#include "HTTP_Request.hpp"
#include "Server.hpp"
#include "sys_calls_warp_arounds.hpp"
#include "WSexception.hpp"
#include "typedefs.hpp"
#include "GlobalContext.hpp"
#include "VirtualServerContext.hpp"

#define MAXIMUM_HTTP_HEADER_SIZE 1024
#define GET_RESPONSE_CONTENT_RAD_BUFFER_SIZE 10000

#define CONNEXION_TIMEOUT_MODE no_timeout
#define IDLE_CLIENT_CONNEXION_TIMEOUT_IN_MILL 3000

#define WEB_RESSOURCES_DIRECTORY "web_ressources"
#define DEFAULT_RESSOURCES_DIRECTORY "./web_ressources/default_pages/"

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
	// constructors and destructors
	ClientConnexionHandler() {}
	ClientConnexionHandler(int socket_connexion_fd, string listening_ip, string listening_port) : 
	FD_interest(IDLE_CLIENT_CONNEXION_TIMEOUT_IN_MILL, CONNEXION_TIMEOUT_MODE, socket_connexion_fd), listening_ip(listening_ip), listening_port(listening_port)
	{
		// cout << "ClientConnexionHandler.listening_ip = " << listening_ip << ", ClientConnexionHandler.listening_port" << listening_port << endl;
	}
	ClientConnexionHandler(const ClientConnexionHandler &other)
	{
		*this = other;
	}
	~ClientConnexionHandler() {}
	// operator overloads
	ClientConnexionHandler &operator=(const ClientConnexionHandler &rhs)
	{
		(void)rhs;
		return *this;
	}

	// methods
	void handle_client_request()
	{
		try
		{
			//set up meber variables
			request = HTTP_Request(fd, MAXIMUM_HTTP_HEADER_SIZE);
			cout << "New request from client on socket " << fd << ":" << endl;
			cout << FAINT_AINSI << request.debug() << END_AINSI << endl;
			virtualServerContext = GlobalContextSingleton.find_corresponding_virtualServerContext(request, listening_ip, listening_port);
			locationContext = virtualServerContext.find_corresponding_location_context(request._path);
			locationContext.apply_to_path();
			//start processing request
			if (request_requires_cgi())
			{
				cout << BLUE_AINSI << "Calling CGI" << END_AINSI << endl;
			}
			else if (request.HTTP_method == "GET")
				start_processing_Get_request("200", request._path);
			else
				throw WSexception("405");
			// if method == POST
			// if already exists overwrite or append?
			// open target ressource AND write on it and construct response AND dedebug reuqest on connexion socket_fd

			// if method == DELETE
			// if file doesn't exist ?
			// delete file and construct response AND dedebug reuqest on connexion socket_fd

			response.set_header_fields("Connection", "Keep-Alive");
		}
		catch (const HTTP_Message::NoBytesToReadException &e)
		{
			std::cout << "Client" << BOLD_AINSI << " closed connexion " << fd << END_AINSI << std::endl;
			close_connexion();
		}
		// make sure to take a reference instead of a copy, otherwise the destructor will be called twice and will potentially call delete twice on the same pointer
		catch (const WSexception &e)
		{
			response = e.response;
			std::cout << "Caught WSexception while processing client request." << std::endl;
			std::cout << "e.what(): " << e.what() << std::endl;
			std::cout << "response: " << std::endl;
			std::cout << response.debug();
			IO_Manager::change_interest_epoll_mask(fd, EPOLLOUT);
		}
		catch (const std::exception &e)
		{
			response = HTTP_Response::Mk_default_response("500");
			std::cerr << RED_AINSI << "ERROR" << END_AINSI << ": Caught Unexpected exception processins request. Setting response to default 500 response. " << std::endl;
			std::cerr << "e.what(): " << e.what() << std::endl;
			IO_Manager::change_interest_epoll_mask(fd, EPOLLOUT);
		}
		if (response.is_error())
			internally_redirect_error_response_to_default_page();
	}
	// GET method
	// open content file AND consturct response from content AND dedebug reuqest on connexion socket_fd
	void start_processing_Get_request(string status_code, string target_ressource_path)
	{
		response = HTTP_Response::mk_from_file_and_status_code(status_code, target_ressource_path);
		// Implement response method that defines response's Content-Type header field.
		IO_Manager::change_interest_epoll_mask(fd, EPOLLOUT);
	}
	bool request_requires_cgi()
	{
		for (size_t i = 0; i < locationContext.cgi_extensions_and_launchers.size(); i++)
		{
			pair<string, string> cgi_extension_and_launcher = locationContext.cgi_extensions_and_launchers[i];
			string extension = cgi_extension_and_launcher.first;
			if (str_end_with(request._path, extension))
			{
				handle_cgi(cgi_extension_and_launcher.second);
				return true;
			}
		}
		return false;
	}
	bool str_end_with(string const &fullString, string const &ending)
	{
		if (fullString.length() >= ending.length())
			return (0 == fullString.compare(fullString.length() - ending.length(), ending.length(), ending));
		return false;
	}

	void handle_cgi(string cgi_launcher)
	{
		char buf[128];

		std::string res = "";
		FILE* otp = popen(this->request._path.c_str(), "r");
		if (!otp)
			throw WSexception("500");
		int	otp_fd = fileno(otp);
		for (; fgets(buf, sizeof(buf), otp) != NULL; res += buf){}

		this->response = HTTP_Response(otp_fd, res.size());
		(void)cgi_launcher;
	}
	void internally_redirect_error_response_to_default_page()
	{
		//If the virtual server context contains an default error page for this
		string response_status_code = response.get_status_code();
		if (virtualServerContext.error_codes_to_default_error_page_path.count(response_status_code))
		{
			string redirected_path = virtualServerContext.error_codes_to_default_error_page_path[response_status_code];
			try
			{
				locationContext = virtualServerContext.find_corresponding_location_context(redirected_path);
				locationContext.apply_to_path(redirected_path);
				start_processing_Get_request(redirected_path);
			}
			catch(const std::exception& e)
			{
				std::cerr << e.what() << '\n';
			}
		}
	}

	void send_response()
	{
		try
		{
			std::cout << "Sending response to client on socket " << fd << ":" << std::endl
					  << FAINT_AINSI << response.debug() << END_AINSI << std::endl;
			ws_send(fd, response.serialize(), 0);
			IO_Manager::change_interest_epoll_mask(fd, EPOLLIN);
		}
		catch (std::exception e)
		{
			std::cerr << "Exception caught in ClientConnexionHandler::" << __func__ << ", client connexion socket fd: " << fd << "." << std::endl;
			std::cerr << "exception.what() = " << e.what() << std::endl;
			std::cerr << "Response: " << response.debug() << std::endl;
			std::cerr << BOLD_AINSI << "Closing" << FAINT_AINSI << " client connexion " << BOLD_AINSI << fd << END_AINSI << "." << std::endl;
			close_connexion();
		}
		response.clear();
		request.clear();
		virtualServerContext = VirtualServerContext();
		locationContext = LocationContext();
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