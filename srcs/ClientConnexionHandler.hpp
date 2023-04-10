#ifndef HTTP_RequestHandler_HPP
#define HTTP_RequestHandler_HPP
#include <sys/wait.h>
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

#define READ 0
#define WRITE 1

#define MAXIMUM_HTTP_HEADER_SIZE 1024
#define GET_RESPONSE_CONTENT_RAD_BUFFER_SIZE 10000

#define CONNEXION_TIMEOUT_MODE no_timeout
#define IDLE_CLIENT_CONNEXION_TIMEOUT_IN_MILL 3000

#define WEB_RESSOURCES_DIRECTORY "web_ressources"
#define DEFAULT_RESSOURCES_DIRECTORY "./web_ressources/default_pages/"

extern GlobalContext GlobalContextSingleton;
extern std::vector<std::string>	g_env;

class ClientHandler : public IO_Manager::FD_interest
{
	private:
	HTTP_Request request;
	HTTP_Response response;
	VirtualServerContext virtualServerContext;
	LocationContext locationContext;
	std::map<int, pid_t> pipe_fds_to_cgi_pids;

	public:
	string listening_ip, listening_port;

	public:
	// constructors and destructors
	ClientHandler() {}
	ClientHandler(int socket_connexion_fd, string listening_ip, string listening_port) : 
	FD_interest(IDLE_CLIENT_CONNEXION_TIMEOUT_IN_MILL, CONNEXION_TIMEOUT_MODE, socket_connexion_fd), listening_ip(listening_ip), listening_port(listening_port)
	{
		// cout << "ClientHandler.listening_ip = " << listening_ip << ", ClientHandler.listening_port" << listening_port << endl;
	}
	ClientHandler(const ClientHandler &other)
	{
		*this = other;
	}
	~ClientHandler() {}
	// operator overloads
	ClientHandler &operator=(const ClientHandler &rhs)
	{
		(void)rhs;
		return *this;
	}

	// methods
	void handle_request()
	{
		try
		{
			//set up meber variables
			virtualServerContext = GlobalContextSingleton.find_corresponding_virtualServerContext(request._hostname, listening_ip, listening_port);
			locationContext = virtualServerContext.find_corresponding_location_context(request._path);
			locationContext.apply_to_path(request._path);

			//start processing request
			string cgi_launcher;
			if (request_requires_cgi(cgi_launcher))
			{
				cout << "Going to generate response from CGI." << endl;
				handle_cgi(cgi_launcher);
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
		}
		// make sure to take a reference instead of a copy, otherwise the destructor will be called twice and will potentially call delete twice on the same pointer
		catch (const WSexception &e) { handle_WSexception(e); }
		catch (const std::exception &e) { handle_unexpected_exception(e); }
	}
	// GET method
	// open content file AND consturct response from content AND dedebug reuqest on connexion socket_fd
	void start_processing_Get_request(string status_code, string target_ressource_path)
	{
		response = HTTP_Response::mk_from_regualr_file_and_status_code(status_code, target_ressource_path);
		// Implement response method that defines response's Content-Type header field.
		IO_Manager::change_interest_epoll_mask(fd, EPOLLOUT);
	}
	bool request_requires_cgi(string& cgi_launcher)
	{
		for (size_t i = 0; i < locationContext.cgi_extensions_and_launchers.size(); i++)
		{
			pair<string, string> cgi_extension_and_launcher = locationContext.cgi_extensions_and_launchers[i];
			string extension = cgi_extension_and_launcher.first;
			if (str_end_with(request._path, extension))
			{
				cgi_launcher = cgi_extension_and_launcher.second;
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

	void	buildCgiCommand(char **cgi_command, std::string cgi_launcher)	{
		if (!cgi_launcher.size())	{
			cgi_command[0] = const_cast<char *>(this->request._path.c_str());
			cgi_command[1] = NULL;
			cgi_command[2] = NULL;
			return;
		}
		cgi_command[0] = const_cast<char *>(cgi_launcher.c_str());
		cgi_command[1] = const_cast<char *>(this->request._path.c_str());
		cgi_command[2] = NULL;
	}

	char	**getEnvToFormatCgi(void)	{
		std::string	QueryStringRequest = ("QUERY_STRING=" + this->request._queryString);
		g_env.push_back(QueryStringRequest);
		char	**envCgi = new char *[g_env.size() + 1];
		size_t i = 0;
		for (; i < g_env.size(); i++)	{
			envCgi[i] = const_cast<char *>(g_env[i].c_str());
		}
		envCgi[i] = NULL;
		return envCgi;
	}	
	void	closeChannelServerCgi(int p_read)	{
		try
		{
			int res = read(p_read, NULL, 0);
			if (res == -1 && errno == EBADF)	{
				close(p_read);
				throw WSexception("403");
			}
			response.partial_constructor_from_fd(p_read);
			if (response.is_fully_constructed)
			{
				cout << "Read CGI response, closing pipe read " << END_AINSI << endl; 
				IO_Manager::remove_interest_and_close_fd(p_read);
				IO_Manager::change_interest_epoll_mask(this->fd, EPOLLOUT);
				wait_cgi(p_read);
			}
			else
				cout << "Constructing response from CGI..." << endl;
		}
		catch(const WSexception& e) { handle_WSexception(e); }
		catch(const std::exception& e) { handle_unexpected_exception(e); }
	}
	void handle_cgi_pipe_hungup(int pipe_fd)
	{
		cout << YELLOW_WARNING << "EPOLLHUP flag set on fd " << fd << ", closing fd." << endl;
		IO_Manager::remove_interest_and_close_fd(pipe_fd);
		response = HTTP_Response::Mk_default_response("500");
		IO_Manager::change_interest_epoll_mask(fd, EPOLLOUT);
		wait_cgi(pipe_fd);
	}
	void wait_cgi(int pipe_fd)
	{
		int cgi_status_code = 0;
		waitpid(pipe_fds_to_cgi_pids[pipe_fd], &cgi_status_code, 0);
		cout << "cgi_status_code: " << cgi_status_code << endl;
		pipe_fds_to_cgi_pids.erase(pipe_fd);
	}
	void	cgiChild(char **cgi_command, std::string cgi_launcher, int *p, int *r)	{
		close(p[READ]);
		dup2(p[WRITE], STDOUT_FILENO);
		close(p[WRITE]);
		dup2(r[READ], STDIN_FILENO);
		close(r[READ]);
		char	**env = this->getEnvToFormatCgi();
		this->buildCgiCommand(cgi_command, cgi_launcher);
		execve(cgi_command[0], cgi_command, env);
		delete [] env;
	}
	void	prepareChannelServerCgi(int *p, int *r)	{
		if (access(this->request._path.c_str(), X_OK))
			throw WSexception("403");
		if (pipe(p) == -1)
			throw WSexception("500");
		if (pipe(r) == -1)	{
			close(p[READ]);
			close(p[WRITE]);
			throw WSexception("500");
		}
	}
	void write_request_on_cgi_stdin(int write_pipe)
	{
		try
		{
			std::string	cRqst = this->request.serialize().c_str();
			if (write(write_pipe, cRqst.c_str(), cRqst.size() + 1) == -1)
				throw WSexception("500");
			IO_Manager::remove_interest_and_close_fd(write_pipe);
		}
		catch (const WSexception& e) { handle_WSexception(e); }
		catch (const std::exception& e) { handle_unexpected_exception(e); }
	}
	void	handle_cgi(const string& cgi_launcher)	{
		char	*cgi_command[3];
		int		p[2];
		int		r[2];

		this->prepareChannelServerCgi(p, r);
		pid_t	pid = fork();
		if (!pid)
			this->cgiChild(cgi_command, cgi_launcher, p, r);
		else if (pid > 0)	{
			close(r[READ]);
			close(p[WRITE]);
			pipe_fds_to_cgi_pids[p[READ]] = pid;
			pipe_fds_to_cgi_pids[r[WRITE]] = pid;
			IO_Manager::set_interest<ClientHandler>(r[WRITE], NULL, &ClientHandler::write_request_on_cgi_stdin, NULL, NULL, -1, no_timeout, this);
			IO_Manager::set_interest<ClientHandler>(p[READ], &ClientHandler::closeChannelServerCgi, NULL, NULL, &ClientHandler::handle_cgi_pipe_hungup, -1, no_timeout, this);
		}
	}

	void send_response()
	{
		if (response.is_error() && !request.is_redirected && virtualServerContext.error_codes_to_default_error_page_path.count(response.get_status_code()))
			internally_redirect_error_response_to_default_page();
		else try
		{
			std::cout << "Sending response to client on socket " << fd << ":" << std::endl
					  << FAINT_AINSI << response.debug() << END_AINSI << std::endl;
			ws_send(fd, response.serialize(), 0);
			IO_Manager::change_interest_epoll_mask(fd, EPOLLIN);
			// request.clear();
			// response.clear();
			request = HTTP_Request();
			response = HTTP_Response();
			virtualServerContext = VirtualServerContext();
			locationContext = LocationContext();
		}
		catch (std::exception e)
		{
			std::cerr << "Exception caught in ClientHandler::" << __func__ << ", client connexion socket fd: " << fd << "." << std::endl;
			std::cerr << "exception.what() = " << e.what() << std::endl;
			std::cerr << "Response: " << response.debug() << std::endl;
			std::cerr << BOLD_AINSI << "Closing" << FAINT_AINSI << " client connexion " << BOLD_AINSI << fd << END_AINSI << "." << std::endl;
			close_connexion();
		}
	}
	void internally_redirect_error_response_to_default_page()
	{
		//If the virtual server context contains an default error page for this
		string redirected_path = virtualServerContext.error_codes_to_default_error_page_path[response.get_status_code()];
		request = HTTP_Request(redirected_path, request);
		// response.clear();
		response = HTTP_Response();
		virtualServerContext = VirtualServerContext();
		locationContext = LocationContext();
		IO_Manager::change_interest_epoll_mask(fd, EPOLLIN);
		cout << BLUE_AINSI << "Handling redirected request, new request is GET " << request._path << END_AINSI << endl;
		handle_request();
	}

	void internal_call_event_callbacks(epoll_event event)
	{
		if ((event.events & EPOLLIN) && (event.events & EPOLLOUT))
			std::cerr << "Warning: called ClientHandler::" << __func__ << " with both EPOLLIN and OUT, this is not supposed to happen!" << std::endl;
		if (event.events & EPOLLOUT)
			send_response();
		if (event.events & EPOLLIN)
		{
			try
			{
				// request = HTTP_Request(fd, MAXIMUM_HTTP_HEADER_SIZE);
				cout << "Constructing request..." << endl;
				request.construct_from_socket(fd);
				if (request.is_fully_constructed)
				{
					cout << "New request from client on socket " << fd << ":" << endl;
					cout << FAINT_AINSI << request.debug() << END_AINSI << endl;
					handle_request();
				}
			}
			catch (const HTTP_Message::NoBytesToReadException &e)
			{
				std::cout << "Client" << BOLD_AINSI << " closed connexion " << fd << END_AINSI << std::endl;
				close_connexion();
			}
		}
	}
	void call_timeout_callback()
	{
		cerr << YELLOW_WARNING << "ClientHandler::" << __func__ << " called but client is not supposed to have timeout!(not closing connexion)" << endl;
	}
	void close_connexion()
	{
		IO_Manager::remove_interest_and_close_fd(fd);
	}

	//Exception handling
	void handle_WSexception(const WSexception& e)
	{
		response = e.response;
		std::cout << "Caught WSexception while processing client request." << std::endl;
		std::cout << "e.what(): " << e.what() << std::endl;
		std::cout << "response: " << std::endl;
		std::cout << response.debug();
		IO_Manager::change_interest_epoll_mask(fd, EPOLLOUT);
		if (request.is_redirected)
		{
			cerr << RED_ERROR << "\tCaught WSexception while handling redirected request, restting response to default 500." << endl;
			cerr << "\tYou probably misstyped the default error page, path pf redirected request: " << request._path << endl;
			response = HTTP_Response::Mk_default_response("500");
		}
	}
	void handle_unexpected_exception(const std::exception& e)
	{
		response = HTTP_Response::Mk_default_response("500");
		std::cerr << RED_AINSI << "ERROR" << END_AINSI << ": Caught Unexpected exception processins request. Setting response to default 500 response. " << std::endl;
		std::cerr << "e.what(): " << e.what() << std::endl;
		IO_Manager::change_interest_epoll_mask(fd, EPOLLOUT);
	}
};
#endif