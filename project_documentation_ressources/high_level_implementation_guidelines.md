# Implementation guidelines
*The sections are ordered by levels of abstractions, lowest at the top, highest at the bottom.*
> ### Sys call warp arounds
> Since we are going to have to make a lot of sys calls, it would be more convinient to have "C++ like" warp arounds functions that throw exceptions instead of returning -1.  
> Instead of writing many ``if (sys_call() == -1) return -1``, we can write one ``try`` ``catch``.


> ### IO Multiplexing management
> ``I/O multiplexing is the the ability to perform I/O operations on multiple file descriptors.``  
> Because of the nature of the project and the constrains given by the subject,  
> multiple IO operations on multiple file descriptors will be performed.  
> To manage those operations and file descriptors we will implement a singleton class,  
> the ``IO_manager``("OMG a singleton class! This is bad practice!" I know, I know).  
> This class will provide to the rest of the code base a convinient, secure and "leak free" solution  
to create/close files and perform IO operations on them.  
> This class will essentially be a big warp around ``epoll``.  
> Insteand of calling ``epoll_ctl`` and ``epoll_wait``,  
> one may simply call ``IO_Manager.add_new_fd_to_interest(new_fd, timout, read_callback, write_callback, timeout_callback)``  
> and the manager will "magically" call the appropriate callbacks when necessary.  
> This way, the caller doesn't need to worry about storing the fd to close it, for timeout and so on.  
> In a way, the ``IO_manager`` raises the rest of the code base to a higher level of abstraction.  
>   
> The manager will contain an associative container ``interest_container`` that will store all the FDs in use as keys and there corresponding callbacks as values.  
> When ``epoll_wait`` returns control, the  manager will, foreach ``epoll_event`` in the ``events`` array,  
find the callbacks corresponding to the FD with the interest_container and call the appropriate ones.  
> 
> ``IO_manager`` will have the following public static methods :
> * ``void add_interest_to_new_fd(new_fd, timout, read_callback, write_callback, timeout_callback, error_callback)``  
*Set ``timeout`` to 0 or less to wait indefinitelly.*
> * ``void modify_interest_to_fd(fd, new_timeout, new_read_callback, new_write_callback, new_timeout_callback, new_error_callback)``  
*Set ``new_timeout`` to 0 not change the previous ``timeout``*
> * ``void Close_fd(fd)``  
*Will remove fd from epoll instance's interest list, the interest_container and close the fd.*

> ### parsing tools
> For now, I don't really know what we will need.  
There is most likely functions that we will use for both the config file and the HTTP request/responses.

> ### Context parsing and browsing
> The gninx's contexts can be thought of as a tree data structure similar to the folder system.  
> The global/main context is the trunc, server contexts are the branches and locations can be both branches and leafs.  
> We will need to both  
> **Make that context structure from the configuration file** and then  
> **browse it to find the appropriate context for a given request**.  
>
> Here is how I see the structure implemented:
> > #### ``template<typename ChildType, std::string name>Context`` *abstract base class*
> > 
> > > ##### fields
> > > * sequential_container of ChildType ``childs``
> > > * strings representing all the directives values
> > 
> > > ##### constructors, destructors and methods
> > > * ``Context(configuration file text(either stream or string))``  
> > >   1. Set its directives to their default values
> > >   2. Set its directives to the values given in the config file (if they are defined)
> > >   3. populates childs with sub configuration file text when it finds an occurence of the child name.  
> > > 
> > > * ``private Context Get_Context_for_request(HTTP request)``  
> > >   1.
