## How to process a request 
---

Once you get woken the server receives a request, it needs to know in which context it will be processed.

### Find the context in which the request will be processed

#### Find the virtual server that will process the request:

1. **Check the URL's IP and port against the ``listen``** directives of the servers.  
*Note:I believe, that the server has to look at the adress:port in the URL of the request instead of deducting them from the socket.  
The adress:port the socket(that recieved the request) was binded to is not necessarly the same as the one the user agent sent the request to.  
Because of ip address forwarding, proxy servers and other stuff I don't understand.  
So we can't just check the adress:port we binded that socket to against the listen directives.*

2. Amongst the servers that have a listen directive that matches the request IP and port, **check the HOST header field against the ``server_names``**.  
If multiple servers's ``server_name`` match the ``HOST`` header field, use the first server that uses the matching ``listen`` directive.

#### Find the location where the request will be processed

1. In the chosen server context, check the **``URI part`` of the request against ``locations``**.
2. Repeat this process recursively as long as there is no more matching ``location`` directive.  

*Note that: ``redirect``, ``root`` and ``index`` directives get applied after the location context has been found.*

### Check for errors

Now that we have the context, we must check if the request complies to it.

#### check if the request complies to the context

- **If** the **request body size is bigger than ``client_max_body_size``**(default is 1MB)
**respond with ``413 Request Entity Too Large``** 
- 