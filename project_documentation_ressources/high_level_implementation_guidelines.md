# High level implementation guidelines
> ### IO Multiplexing management
> ``I/O multiplexing is the the ability to perform I/O operations on multiple file descriptors.`` 
Because of the nature of the project and the constrains given by the subject,  
multiple IO operations on multiple file descriptors will be performed.  
To manage those operations and file descriptors we will implement a singleton class,  
the ``IO_MultiplexingManager``("OMG a singleton class! This is bad practice!" I know, I know).  
This class will provide to the rest of the code base a convinient and secure way to perform those operations.
Effectively, raising the rest of the code base to a higher level of abstraction.  
This class will be a big warp around low level system calls like ``socket``, ``listen``, and most importantly ``epoll``.  
