## How to process a request 
---
### Choose the virtual server that will process the request:

1. **Check the URL's IP and port against the ``listen``** directives of the servers.  
I believe, that the server has to look at the adress:port in the URL of the request instead of deducting them from the socket.
The adress:port the socket was binded is not necessarly the same as the one the user agent sent the request to.  
Because of ip address forwarding and other stuff I don't understand.
2. Amongst the servers that have a listen directive that matches the request IP and port, **check the HOST header field against the ``server_names``**.  
If multiple servers's ``server_name`` match the ``HOST`` header field, use the first server that uses the matching ``listen`` directive.

### Choose the location where the request will be processed

1. In the chosen server context, check the **``URI part`` of the request against ``locations``**.
2. Repeat this process recursively as long as there is no more matching ``location`` directive.  

*Note that: ``redirect``, ``root`` and ``index`` directives get applied after the location context has been found.*