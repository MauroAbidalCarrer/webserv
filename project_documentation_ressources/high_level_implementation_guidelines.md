# High level implementation guidelines
> ### IO Multiplexing management
> ``I/O multiplexing is the the ability to perform I/O operations on multiple file descriptors.``  
> Because of the nature of the project and the constrains given by the subject,  
> multiple IO operations on multiple file descriptors will be performed.  
> To manage those operations and file descriptors we will implement a singleton class,  
> the ``IO_manager``("OMG a singleton class! This is bad practice!" I know, I know).  
> This class will provide to the rest of the code base a convinient, secure and "leak free" way to create/close files and perform IO operations on them.  
> This class will be a big warp around ``epoll``.  
> Insteand of calling ``epoll_ctl`` and ``epoll_wait``,  
> one may simply call ``IO_Manager.add_new_fd_to_interest(new_fd, timout, read_callback, write_callback, timeout_callback)``  
> and the manager will "magically" call the appropriate callbacks when necessary.  
> This way, the caller doesn't need to worry about storing the fd to close it, for timeout and so on.  
> In a way, the ``IO_manager`` raises the rest of the code base to a higher level of abstraction.  
>   
> The manager will contain a map ``interest_map`` that will store all the file descriptors in use.  
> FDs will be the keys of the map.
> The values will be a set of callbacks associated with each I/O operations to perform on the FD.  
> When ``epoll_wait`` returns control, the  manager will, foreach ``epoll_event`` in the ``events`` array,  
find the callbacks corresponding to the FD with the interest_map and call the appropriate ones.  
> 
> ``IO_manager`` will have the following functionalities :
> * ``void add_interest_to_new_fd(new_fd, timout, read_callback, write_callback, timeout_callback, error_callback)``  
*Set ``timeout`` to 0 or less to wait indefinitelly.*
> * ``void modify_interest_to_fd(fd, new_timeout, new_read_callback, new_write_callback, new_timeout_callback, new_error_callback)``  
*Set ``new_timeout`` to 0 not change the previous ``timeout``*
> * ``void Close_fd(fd)``  
*Will remove fd from epoll instance's interest list, the interest_map and close the fd.*