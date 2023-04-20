#ifndef HTTP_RequestHandler_HPP
#define HTTP_RequestHandler_HPP
#include <sys/wait.h>
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>
#include <signal.h>

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

#define CGI_TIMEOUT_IN_MILL 2000
#define REQUEST_TIMEOUT_IN_MILL 1000

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
	vector<pair<string, string> > url_encoded_collector;

	public:
	string listening_ip, listening_port;

	public:
	// constructors and destructors
	ClientHandler() {}
	ClientHandler(int socket_connexion_fd, string listening_ip, string listening_port) : 
	FD_interest(IDLE_CLIENT_CONNEXION_TIMEOUT_IN_MILL, CONNEXION_TIMEOUT_MODE, socket_connexion_fd), listening_ip(listening_ip), listening_port(listening_port)
	{ }
	ClientHandler(const ClientHandler &other)
	{
		*this = other;
	}
	~ClientHandler() {}
	// operator overloads
	ClientHandler &operator=(const ClientHandler &rhs)
	{
		request = rhs.request;
		response = rhs.response;
		virtualServerContext = rhs.virtualServerContext;
		locationContext = rhs.locationContext;
		pipe_fds_to_cgi_pids = rhs.pipe_fds_to_cgi_pids;
		url_encoded_collector = rhs.url_encoded_collector;
		return *this;
	}

	// member methods
	void handle_request()
	{
		try
		{
			string cgi_launcher;

			//set up meber variables
			virtualServerContext = GlobalContextSingleton.find_corresponding_virtualServerContext(request._hostname, listening_ip, listening_port);
			locationContext = virtualServerContext.find_corresponding_location_context(request._path);
			locationContext.apply_to_path(request._path);
			//verify that request conforms to contexts constrains
			if (std::count(locationContext.allowed_methods.begin(), locationContext.allowed_methods.end(), request.HTTP_method) == 0)
				throw WSexception("405");
			if (request.content_length > virtualServerContext.client_body_size_limit)
				throw WSexception("413");
			if (virtualServerContext.redirected_URLs.count(request._path))
			{
				string redirected_url = virtualServerContext.redirected_URLs[request._path];
				HTTP_Response::set_redirection_response(response, redirected_url);
				cout << BLUE_AINSI << "Redirecting request with path \"" << request._path << "\" to \"" << redirected_url << "." << END_AINSI << endl; 
				IO_Manager::change_interest_epoll_mask(fd, EPOLLOUT);
			}
			//start processing request
			else if (request_requires_cgi(cgi_launcher))
			{
				cout << "Going to generate response from CGI." << endl;
				handle_cgi(cgi_launcher);
			}
			else if (request.HTTP_method == "GET")
				process_GET_request("200", request._path);
			else if (request.HTTP_method == "POST")
				process_POST_request("201", request._path);
			else if (request.HTTP_method == "DELETE")
				process_DELETE_request();
			else
				throw WSexception("405");
		}
		// make sure to take a reference instead of a copy, otherwise the destructor will be called twice and will potentially call delete twice on the same pointer
		catch (const WSexception &e) { handle_WSexception(e); }
		catch (const std::exception &e) { handle_unexpected_exception(e); }
	}

	//POST mehtod
	void	treat_multipart_body_boundaries(std::string body)	{
		(void)body;
	}
	void	treat_encoded_url(std::string body)	{
		size_t	value = 0;
		for (size_t key = 0; key != std::string::npos || value != std::string::npos; )	{
			key = body.find("=", 0);
			value = body.find("&", key);
			url_encoded_collector.push_back(std::make_pair(body.substr(0, key), body.substr(key + 1, value - key - 1)));
			if (value > body.size())
				break ;
			if (value != std::string::npos)
				body.erase(0, value + 1);
		}
	}
	void	add_user_in_db()	{
		std::ofstream db;
		db.open("web_ressources/users/all", std::ios_base::app);
		if (!db.is_open())	{
			return ;
		}
		db << url_encoded_collector[0].second;
		db << " | ";
		db << url_encoded_collector[1].second << endl;
		db.close();
	}
	bool	search_user_in_db()	{
		std::fstream db;
		db.open("web_ressources/users/all", std::fstream::in | std::ios_base::app);
		if (!db.is_open())	{
			return false ;
		}
		std::string	db_content;
		std::string	format = " | ";
		while (getline(db, db_content))	{
			if (!url_encoded_collector[0].second.compare(db_content.substr(0, url_encoded_collector[0].second.size())))	{
				if (!url_encoded_collector[1].second.compare(db_content.substr(url_encoded_collector[0].second.size() + format.size(), db_content.size())))
					return db.close(), true;
				else
					cout << RED_AINSI << "POST: [BAD PASSWORD FOR CONNEXION]" << END_AINSI << endl;
			}
		}
		return  db.close(), false;
	}
	unsigned int	redirect_post_request_on_content_type(vector<string> content_type)	{
		if (!content_type[1].compare("multipart/form-data;"))	{
			treat_multipart_body_boundaries(std::string(request.body));
			return 0;
		}
		else if (!content_type[1].compare("application/x-www-form-urlencoded"))	{
			treat_encoded_url(std::string(request.body));
			return 1;
		}
		else
			return 2;
	}
	void	process_POST_request(std::string status_code, string target_ressource_path)	{
		vector<string>	content_type = request.get_header_fields("Content-Type");
		switch (redirect_post_request_on_content_type(content_type))	{
			case 0:
				treat_multipart_body_boundaries(string(request.body));
				response = HTTP_Response::mk_from_regualr_file_and_status_code(status_code, target_ressource_path);
				break;
			case 1:
				if (!url_encoded_collector[0].first.compare("connexion-page-email") && search_user_in_db())	{
					target_ressource_path = "web_ressources/logged.html";
				}
				else if (!url_encoded_collector[0].first.compare("subscribe-page-email") )	{
					add_user_in_db();
				}
				response = HTTP_Response::mk_from_regualr_file_and_status_code(status_code, target_ressource_path);
				url_encoded_collector.clear();
				break;
			case 2:
				response = HTTP_Response::mk_from_regualr_file_and_status_code(status_code, target_ressource_path);	}
		IO_Manager::change_interest_epoll_mask(fd , EPOLLOUT);
	}
	// GET method
	// open content file AND consturct response from content AND dedebug reuqest on connexion socket_fd
	void process_GET_request(string status_code, string target_ressource_path)
	{
		if (is_directory(request._path))
		{
			if (locationContext.directory_listing == false)
				throw WSexception("403");
			cout << "Constructing directory listing response for request." << endl;
			HTTP_Response::set_directory_listing_response(response, request._path);
		}
		else if(is_regular_file(request._path))
			response = HTTP_Response::mk_from_regualr_file_and_status_code(status_code, target_ressource_path);
		else
			response = HTTP_Response::Mk_default_response("404");
		IO_Manager::change_interest_epoll_mask(fd, EPOLLOUT);
	}
	void process_DELETE_request()
	{
		delete_file(request._path);
		response = HTTP_Response::Mk_default_response("200");
		IO_Manager::change_interest_epoll_mask(fd, EPOLLOUT);
	}

	//CGI
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
	void	closeChannelServerCgi(int p_read)
	{
		PRINT("Constructing response from CGI...");
		try
		{
			int res = read(p_read, NULL, 0);
			if (res == -1 && errno == EBADF)	{
				IO_Manager::remove_interest_and_close_fd(p_read);
				throw WSexception("403");
			}
			response.construct_from_CGI_output(p_read);
			if (response.is_fully_constructed)
			{
				cout << "Fully constructed CGI response, closing pipe read and waiting for child process." << END_AINSI << endl; 
				IO_Manager::remove_interest_and_close_fd(p_read);
				IO_Manager::change_interest_epoll_mask(this->fd, EPOLLOUT);
				wait_cgi(p_read);
			}
			else
			{
				PRINT("Response from CGI not yet constructed...");
				PRINT_FAINT("respnse: " << response.serialize());
			}
		}
		catch(const WSexception& e) { handle_WSexception(e); }
		catch(const std::exception& e) { handle_unexpected_exception(e); }
	}
	void handle_cgi_read_pipe_hungup(int pipe_fd)
	{
		cout << YELLOW_WARNING << "EPOLLHUP flag set on read pipe " << fd << ", closing fd and setting response to default 500." << endl;
		IO_Manager::remove_interest_and_close_fd(pipe_fd);
		response = HTTP_Response::Mk_default_response("500");
		IO_Manager::change_interest_epoll_mask(fd, EPOLLOUT);
		wait_cgi(pipe_fd);
	}
	void handle_cgi_timeout(int p_read)
	{
		if (pipe_fds_to_cgi_pids.count(p_read) == 0)
			throw runtime_error("\"pipe_fds_to_cgi_pids\" does not contain read pipe when handlign CGI timeout");
		PRINT_WARNING("CGI with pid " << pipe_fds_to_cgi_pids[p_read] << " timed out, going to terminate it.");
		IO_Manager::remove_interest_and_close_fd(p_read);
		if (kill(pipe_fds_to_cgi_pids[p_read], SIGTERM) == -1)
			throw runtime_error("Could not terminate CGI that timed out");
		pipe_fds_to_cgi_pids.erase(p_read);
		response = HTTP_Response::Mk_default_response("504");
		IO_Manager::change_interest_epoll_mask(fd, EPOLLOUT);
	}
	void wait_cgi(int pipe_fd)
	{
		int cgi_status_code = 0;
		pipe_fds_to_cgi_pids.erase(pipe_fd);
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
		PRINT_ERROR(RED_AINSI << "failed to execve CGI, path" << END_AINSI);
		delete [] env;
		close(r[WRITE]);
		throw StopWaitLoop();
	}
	void	prepareChannelServerCgi(int *p, int *r)	{
		if (access(this->request._path.c_str(), F_OK))
			throw WSexception("404");
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
			PRINT("wrote request to cgi.");
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
		pid_t	cgi_pid = fork();
		if (cgi_pid == -1)
			throw runtime_error("Could not fork to launch CGI, WTF\?!");
		else if (!cgi_pid)
		{
			g_pid = getpid();
			PRINT("New child process, parent pid: " << getppid());
			this->cgiChild(cgi_command, cgi_launcher, p, r);
		}
		else if (cgi_pid > 0)
		{
			ws_close(r[READ], "Closing read end of CGI stdout pipe in parent");
			ws_close(p[WRITE], "Closing write end of CGI stdin pipe in parent");
			pipe_fds_to_cgi_pids[p[READ]] = cgi_pid;
			IO_Manager::set_interest<ClientHandler>(r[WRITE], NULL, &ClientHandler::write_request_on_cgi_stdin, NULL, NULL, -1, no_timeout, this);
			IO_Manager::set_interest<ClientHandler>(p[READ], &ClientHandler::closeChannelServerCgi, NULL, &ClientHandler::handle_cgi_timeout, &ClientHandler::handle_cgi_read_pipe_hungup, CGI_TIMEOUT_IN_MILL, do_not_renew, this);
			// response = HTTP_Response::Mk_default_response("200");
			// IO_Manager::change_interest_epoll_mask(fd, EPOLLOUT);
		}
	}

	void send_response()
	{
		if (response.is_error() && !request.is_redirected && virtualServerContext.error_codes_to_default_error_page_path.count(response.get_status_code()))
			internally_redirect_error_response_to_default_page();
		else try
		{
			PRINT("Sending response to client on socket " << fd << ":" << std::endl
					  << FAINT_AINSI << response.debug() << END_AINSI);
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
		PRINT(BLUE_AINSI << "Handling redirected request, new request is GET " << request._path << END_AINSI);
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
				// PRINT("Setting timeout mode to " << renew_after_IO_operation);
				timeout_mode = renew_after_IO_operation;
				timeout_in_mill = REQUEST_TIMEOUT_IN_MILL;
				// request = HTTP_Request(fd, MAXIMUM_HTTP_HEADER_SIZE);
				request.construct_from_socket(fd);
				if (request.is_fully_constructed)
				{
					timeout_mode = no_timeout;
					PRINT("New request from client on socket " << fd << ":");
					PRINT_FAINT(request.serialize());
					PRINT_FAINT("request.body.size: " << request.body.size());
					handle_request();
				}
			}
			catch (const HTTP_Message::NoBytesToReadException &e) { close_connexion(); }
			catch (const WSexception& e) { handle_WSexception(e); }
			catch (const std::exception& e) { handle_unexpected_exception(e); }
		}
	}
	void call_timeout_callback()
	{
		// cerr << YELLOW_WARNING << "ClientHandler::" << __func__ << " called but client is not supposed to have timeout!(not closing connexion)" << endl;
		PRINT_WARNING("Connexion on socket " << fd << " timed out.");
		close_connexion();
	}
	void close_connexion()
	{
		PRINT("Closing client connexion on socket " << fd);
		IO_Manager::remove_interest_and_close_fd(fd);
	}

	//Exception handling
	void handle_WSexception(const WSexception& e)
	{
		response = e.response;
		PRINT("Caught WSexception while processing client request.");
		PRINT("e.what(): " << e.what());
		PRINT("response: ");
		PRINT(response.debug());
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
		std::cerr << RED_AINSI << "ERROR" << END_AINSI << ": Caught Unexpected exception while processing request. Setting response to default 500 response. " << std::endl;
		std::cerr << "e.what(): " << e.what() << std::endl;
		IO_Manager::change_interest_epoll_mask(fd, EPOLLOUT);
	}
};
#endif