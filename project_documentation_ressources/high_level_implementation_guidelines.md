# High level implementation guidelines
> ### IO Multiplexing management
> ``I/O multiplexing is the the ability to perform I/O operations on multiple file descriptors.``  
> Because of the nature of the project and the constrains given by the subject,  
> multiple IO operations on multiple file descriptors will be performed.  
> To manage those operations and file descriptors we will implement a singleton class,  
> the ``IO_MultiplexingManager``("OMG a singleton class! This is bad practice!" I know, I know).  
> This class will provide to the rest of the code base a convinient, secure and *leak free* way to create/close files and perform IO operations on them.  
> This class will be a big warp around low level system calls.  
> Instead of calling ``socket``, ``bind``, ``listen`` and ``epoll_wait`` and then figure out what action to perform based on the epoll_event one may simply call ``IO_MultiplexingManager.new_listenning_socket()``  
> Effectively, raising the rest of the code base to a higher level of abstraction.  
>   
> The manager will contain a map that will store all the file descriptors in use.  
> FDs will be the keys of the map.
> The values will be a set of callbacks associated with each I/O operations.  
> Here are the methods it will provide:
> * ``void add()