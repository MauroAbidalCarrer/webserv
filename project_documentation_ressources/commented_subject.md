## Requested features for configuration file clarified:

- Choose the port and host of each ’server’.  
    >means: Implment the [listen](http://nginx.org/en/docs/http/ngx_http_core_module.html#listen) directive.
- Setup the server_names or not.
    >means: Implment the [server_name](http://nginx.org/en/docs/http/ngx_http_core_module.html#server_name) directive.
- The first server for a host:port will be the default for this host:port (that means 
it will answer to all the requests that don’t belong to an other server).
- Setup default error pages.
- Limit client body size.
    >means: Implment the [client_max_body_size](http://nginx.org/en/docs/http/ngx_http_core_module.html#client_max_body_size) directive.
- Setup routes with one or multiple of the following rules/configuration (routes wont be using regexp):
    >means: Implment the [location](http://nginx.org/en/docs/http/ngx_http_core_module.html#location) context without regexp expression.  
    - Define a list of accepted HTTP methods for the route.
    - Define a HTTP redirection.
    - Define a directory or a file from where the file should be searched (for example, if url /kapouet is rooted to /tmp/www, url /kapouet/pouic/toto/pouet is /tmp/www/pouic/toto/pouet).
    - Turn on or off directory listing.
    - Set a default file to answer if the request is a directory.
    - Execute CGI based on certain file extension (for example .php).
    - Make it work with POST and GET methods.
    - Make the route able to accept uploaded files and configure where they should be saved.
        - Do you wonder what a CGI is?
        >⚠️ The Wikipedia page is not enough for in it of itself, I recommend you read [this documention](https://www.oreilly.com/library/view/cgi-programming-on/9781565921689/04_chapter-01.html) from chapter one to three.
        - Because you won’t call the CGI directly, use the full path as PATH_INFO.
        - Just remember that, for chunked request, your server needs to unchunk it, the CGI will expect EOF as end of the body.
        - Same things for the output of the CGI. If no content_length is returned from the CGI, EOF will mark the end of the returned data.
        - Your program should call the CGI with the file requested as first argument.
        - The CGI should be run in the correct directory for relative path file access.
        - Your server should work with one CGI (php-CGI, Python, and so forth).
        > I believe they meant to say that your server should work with one ***type*** of CGI.