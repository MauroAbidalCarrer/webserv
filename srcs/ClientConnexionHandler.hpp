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

# define MAXIMUM_HTTP_HEADER_SIZE 1024
# define GET_RESPONSE_CONTENT_RAD_BUFFER_SIZE 10000

# define CONNEXION_TIMEOUT_MODE no_timeout
# define IDLE_CLIENT_CONNEXION_TIMEOUT_IN_MILL 3000

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
        }
        catch (std::exception& e)
        {
            std::cerr << "Caught exception while construcintg client request, closing client connexion " << fd << std::endl;
            close_connexion();
        }
        //find context
        //apply context
        if (*(request.target_URL.end() - 1) == '/')
            request.target_URL.append("index.html");
        //if request requires CGI generate response
            //fork, excve CGI, input request to CGI input AND read from CGI output, complete header fields
        if (request.HTTP_method == "GET")
            start_processing_Get_request();
        //if method == POST
            //if already exists overwrite or append?
            //open target ressource AND write on it and construct response AND deserialize reuqest on connexion socket_fd
        //if method == DELETE
            //if file doesn't exist ?
            //delete file and construct response AND deserialize reuqest on connexion socket_fd
        parsing::line_of_tokens_t connection_header;
        response.set_header_fields("Connection", "Keep-Alive");
    }
    //GET method
    //open content file AND consturct response from content AND deserialize reuqest on connexion socket_fd
    void start_processing_Get_request()
    {
        int content_fd = ws_open(request.target_URL, O_RDONLY);
        //set response Content-Type
        IO_Manager::change_interest_epoll_mask(fd, 0);
        IO_Manager::set_interest<ClientConnexionHandler>(content_fd, &ClientConnexionHandler::append_to_GET_response_content, NULL, &ClientConnexionHandler::construct_GET_response_from_content, 0, do_not_renew, this);
    }
    void append_to_GET_response_content(int content_fd)
    {
        response.body.append(ws_read(content_fd, GET_RESPONSE_CONTENT_RAD_BUFFER_SIZE));
    }
    void construct_GET_response_from_content(int content_fd)
    {
        IO_Manager::remove_interest_and_close_fd(content_fd);
        std::ostringstream convert;
        convert << response.body.length();
        response.set_header_fields("Content-Length", convert.str());
        response.set_header_fields("Content-Type", "text/html;");
        IO_Manager::change_interest_epoll_mask(fd, EPOLLOUT);
    }
    
    void send_response()
    {
        ws_send(fd, response.deserialize(), 0);
        IO_Manager::change_interest_epoll_mask(fd, EPOLLIN);
        
    }

    void internal_call_event_callbacks(epoll_event event)
    {
        if ((event.events & EPOLLIN) && (event.events & EPOLLOUT))
            std::cerr << "Warning: called ClientConnexionHandler::" << __func__ << " with both EPOLLIN and OUT, this is not supposed to happen!" << std::endl;
        if (event.events & EPOLLIN)
            handle_client_request();
        if (event.events & EPOLLOUT)
            send_response();
    }
    void call_timeout_callback()
    {
        std::cerr << "Warning: ClientConnexionHandler::" << __func__ << " called but client is not supposed to have timeout!" << std::endl;
    }
    void close_connexion()
    {
        IO_Manager::remove_interest_and_close_fd(fd);
    }
};
#endif