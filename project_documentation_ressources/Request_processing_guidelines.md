# Request processing guidelines 

### Receive the request
> This is the best solution I found that respects ALL the constrians of the subject:  
> Do a ``recv`` calls and epoll_waits of with a 0 timeout as long as the number of bytes read by recv is inferior to the buffer size.  
> *Note: Ideally we would simply call recv with the flag ``MSG_DONWAIT`` until recv returns -1 and that ``(errno | EAGAIN) (errno | EWOULDBLOCK)``.*  
> *But we are not allowed to check errno after read/write operations (-_-).*  
### Find the context in which the request will be processed
> Once the server receives a request, it first needs to know in which context it will be processed in.
> 
> #### Find the virtual server that will process the request:
> 1. **Check the URL's IP and port against the ``listen``** directives of the servers.  
> *Note: I believe, that the server has to look at the adress:port in the URL of the request instead of deducting them from the socket.  
> The adress:port the socket(that recieved the request) was binded to is not necessarly the same as the one the user agent sent the request to.  
> Because of ip address forwarding, proxy servers and other stuff I don't understand.  
> So we can't just check the adress:port we binded that socket to against the listen directives.*
> 
> 2. Amongst the servers that have a listen directive that matches the request IP and port, **check the HOST header field against the ``server_names``**.  
> If multiple servers's ``server_name`` match the ``HOST`` header field, use the first server that uses the matching ``listen`` directive.
> 
> #### Find the location where the request will be processed
> 
> 1. In the server context, **check the ``URI part`` of the request against ``location``s directives**.
> 2. Repeat this process recursively until there is no more matching ``location`` directive.  
> 
> *Note that: ``redirect``, ``root`` and ``index`` directives get applied after the location context has been found.*  
> *We could find the context as soon as we read the first line of the HTTP request and then stop reading the request as soon as we found an error speciffic to the context.*  
> *That would make the server a bit more performant as it would not waist time reciving a request it will discard because of context/request error, but it would also make it a lot more complicated to implement.*  


### Check for errors relative to the context
> Now that we have the context, we must check if the request complies to it.
> 
> #### Check if the request complies to the context
> 
> - **If** the **request body size is bigger than ``client_max_body_size``**(default is 1MB), **respond with ``413 Request Entity Too Large``** 
> - **If** the request **method is not allowed** by the ``allowed_methods`` directive, **respond with ``405 Method Not Allowed``**  
>
> **If** the server must respond with an **error and if context defines a default error page for that error code** with the ``error_page`` directive,
**return that page**.

### Apply modifications to the requested path according to the context
> - Apply the ``root`` directive by inserting the path of ``root`` at the beggining of the path file.
> - Apply the ``return``(or rewrite?) directive if it is defined to redirect the user agent.
> - If the ``index`` directive is defined, add its value to the end of the path.

### Genereate and send response
> Methods specific steps: 
> > #### ``GET`` method: 
> > If ``ngx_http_autoindex_module`` is on and the requested path is a directory, respond with the list of files and directories in the requested directory.  
> > If it is not defined or if it is defined as false don't respond with directory listing.  
> > If the file does exist, open it, read it(with a read/wait loop) and send it in the response.
>  
> > #### ``POST`` method
>  
> > #### ``DELETE`` method
> 
> > #### CGI steps
> > If the path is a path to a file and that this file extension matches the one defined by the directive ``cgi_pass``(or ``cgi_setup``?)
> > 1. Create pipes to read and write to std_out and in of the CGI that is going to be executed.
> > 2. Fork
> >       * **In child**:  
> >           1. dup2 and close the pipes.  
> >           2. Try to excve the file. If it is not possible to excecute the file, respond with and error 401, 403, 404 or 500 depending on the error encountered.
> >       * **In parent**:
> >           1. close the unused pipe ends.  
> >           2. Wait to be able to write on the STDIN pipe, then write the request on the STDIN pipe.
> >           3. Wait to be able to read from the STDOUT pipe, then read the request on the STDOUT pipe  
*maybe read the CGI output the same way we read the client request?*.
> >           4. ``waitpid`` the CGI with ``NOHANG`` (or kill it?).
> >           4. Complete the response with any missing header field.  
>
> If an error has been encountered, close the connexion.