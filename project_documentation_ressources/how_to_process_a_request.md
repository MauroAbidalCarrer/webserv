# How to process a request 

### Receive the request
> This is the best solution I found that respects ALL the constrians of the subject:
> We need to parse the request as we receive it to know when we can stop calling recv.
> > According to the [RFC](https://www.rfc-editor.org/rfc/rfc7230#section-3.3.3) here is how to process the message:
> > - Read from the socket until you encounter the ``\r\n\r\n`` sequence of bytes denoting the end of the header.
> > - If the method of the request does not have a body(like ``GET``), we know we received the full request.
> > - If a message is received with both a Transfer-Encoding and a Content-Length header field,  
such a message might indicate an attempt to perform request smuggling (Section 9.5) or response splitting (Section 9.4)  
and ought to be handled as an error (*400 Bad Request?*).
> > - If a Transfer-Encoding header is present and has a value other than identity, read the message body in chunks until a 0-length chunk is read(see [RFC]()).
> > - If a Content-Length header is present, read from the socket until the exact number of bytes specified have been read.
> > - (*note sure we should implement this one*)If the Content-Type header indicates a multipart/... media type, read from the socket and parse the MIME data until the final terminating MIME boundary is reached.  
> > *Note: For every recv/read loops that gets terminated by reading a delimiter we should set a maximum amount of bytes from which we consider the request to be invalid.  
to prevent from looping for ever in case of a maliscious user agent that sends infinite amounts of data without any delimiter
> 
> > *Note:*  
> > *Ideally we would simply call recv with the flag ``MSG_DONWAIT`` until recv returns -1 and that ``(errno | EAGAIN) (errno | EWOULDBLOCK)``.*  
> > *But we are not allowed to check errno after read/write operations (-_-).*  
> > *It's not possible to perform ``recv`` and ``epoll_wait`` calls until the number of read bytes is inferior to the buffer size.*  
> > *Because the server would do an extra ``epoll_wait`` if the request/message length is a multiple of the buffer size and would hang foreever.*

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


### Check for errors relative to the context
> Now that we have the context, we must check if the request complies to it.
> 
> #### check if the request complies to the context
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

### Genereate response
> If ``autoindex`` is on and the requested path is a directory, respond with the list of files and directories in the requested directory.  
> If it is not defined or if it is defined as false don't do it.