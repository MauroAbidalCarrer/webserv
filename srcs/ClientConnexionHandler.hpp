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
            
            find_and_apply_contet_to_request();
            /*if request requires CGI generate response
                
                fork, excve CGI, AND input request to CGI input AND read from CGI output, complete header fields AND send response
            */
            if (request.HTTP_method == "GET")
                start_processing_Get_request();
            //if method == POST
                //if already exists overwrite or append?
                //open target ressource AND write on it and construct response AND deserialize reuqest on connexion socket_fd
            //if method == DELETE
                //if file doesn't exist ?
                //delete file and construct response AND deserialize reuqest on connexion socket_fd
            response.set_header_fields("Connection", "Keep-Alive");
        }
        catch (HTTP_Message::NoBytesToReadException e)
        {
            std::cout << "No more bytes to read on client socket " << fd << ", closing client connexion " << fd << std::endl;
            close_connexion();
        }
        catch(SystemCallException e)
        {
            response.clear();
            response.first_line.push_back("500");
            response.first_line.push_back("Internal Server Error");
            // response = HTTP_Response("500", "Internal Server Error");
            if (e.sys_call_name == "open")
            {
                if (e.system_call_errno & ENOENT)
                    response = HTTP_Response("404", "Not Found");
                else if (e.system_call_errno & EACCES)
                    response = HTTP_Response("406", "Access to this resource on the server is denied");
            }
            std::cerr << "SystemCallException caught while starting to process request: " << std::endl;
            std::cerr << "- " << e.what() << std::endl;
            std::cerr << "- Response: " << std::endl;
            std::cerr << response.deserialize();
            std::cerr << "-----------" << std::endl;
            IO_Manager::change_interest_epoll_mask(fd, EPOLLOUT);
        }
    }
    //methods
    void  find_and_apply_contet_to_request()
    {
        //find context
        //apply context
        //apply root directive(for no it just adds .)
        request.target_URL.insert(request.target_URL.begin(), '.');
        //apply rewrite directive(not sure if it's the rewrite directive... the that completes target URLs finishing ini "/")(for no just index.html)
        if (*(request.target_URL.end() - 1) == '/')
            request.target_URL.append("index.html");
    }
    //GET method
    //open content file AND consturct response from content AND deserialize reuqest on connexion socket_fd
    void start_processing_Get_request()
    {
        int content_fd = ws_open(request.target_URL, O_RDONLY);
        //set response correct Content-Type
        //make read_full method
        response.body = ws_read(content_fd, 10000);
        response.body.append(parsing::CLRF);
        response.set_content_length();
        //Implement response method that defines response's Content-Type header field.
        response.set_header_fields("Content-Type", "text/html;");
        response.set_response_line("200", "OK");
        IO_Manager::change_interest_epoll_mask(fd, EPOLLOUT);
    }

    void send_response()
    {
        ws_send(fd, response.deserialize(), 0);
        response.clear();
        request.clear();
        IO_Manager::change_interest_epoll_mask(fd, EPOLLIN);
    }

    void internal_call_event_callbacks(epoll_event event)
    {
        if ((event.events & EPOLLIN) && (event.events & EPOLLOUT))
            std::cerr << "Warning: called ClientConnexionHandler::" << __func__ << " with both EPOLLIN and OUT, this is not supposed to happen!" << std::endl;
        std::cout << "ClientConnexionHandler::" << __func__ << ", event.events = " << event.events << ", fd = " << fd << std::endl;
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