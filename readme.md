## webserv

This is a project to prepare to the 42 webserv project.  
We are going to implement a static webserver that will only implement the GET request.  
The server will be configurable through the use of ngnix like configuration files, although it will most likely not implement every feature requested in the subject.  
The server will follow the webserv [subject](/project_documentation_ressources/subject.pdf) for the feature it will implement to ease the exportation of te code to the final webserv project.  
Because the official school subject uses a lot of esoteric termes difficult to understand for newcomers I've made a [commented version](/project_documentation_ressources/commented_subject.md) of the subject.  

I have also written a guide on how to [process a request](/project_documentation_ressources/Request_processing_guidelines.md) as well as [implementation guidelines](/project_documentation_ressources/high_level_implementation_guidelines.md).


Ressources to get started on the project:
- [what is a URL?](https://developer.mozilla.org/en-US/docs/Learn/Common_questions/Web_mechanics/What_is_a_URL#basics_anatomy_of_a_url)
- [How do server_names work?](http://nginx.org/en/docs/http/server_names.html)
- [nginx config examles](https://github.com/JCluzet/webserv/blob/main/config/default.conf)
- [response status codes](https://httpwg.org/specs/rfc9110.html#status.code.registration)
- [MIME CSVs](https://www.iana.org/assignments/media-types/media-types.xhtml)
- [status codes CSV](https://github.com/Mr-Pi/httpStatusCodes/blob/master/priv/http-status-codes-1.csv)
