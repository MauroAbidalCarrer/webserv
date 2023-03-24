/*
    Sys call warp arounds are functions that will call the sys call and throw an exception in case the sys call fails.
    Sys call also take an additional argument, a message that will be added into the string returned by the exception.what method.
    This will give more context for debugging puposes in case of a failure.

    Sys calls warp arounds have the same name as the sys call + a prefixe ws_ for WebServer(e.g: epoll_wait becomes ws_epoll_wait).
    Having such sys call warpers guarantees that no error has been encountered when they return control.
    This prevents extensive error handling in the rest of the code, making it shorter and more readable.
*/
#ifndef UTILS
# define UTILS
# include <iostream>
# include <string>
# include <vector>
# include <sys/epoll.h>
# include <sys/fcntl.h>
# include <stdio.h>
# include <cerrno>
# include <stdlib.h>
# include <unistd.h>
# include <sys/types.h>
# include <sys/socket.h>
# include <string>
# include <string.h>
# include <netinet/in.h>
# include <algorithm>
# include <iterator>
# include <list>
# include <cstdlib>
# include <sys/epoll.h>
# include <time.h>
# include <sys/time.h>



//Exception thrown when a sys call fails.
class SystemCallException : public std::exception
{
    private:
    std::string errormsg;

    public:
    SystemCallException(std::string sys_call_name)
    {
        errormsg = sys_call_name + ": " + std::string(strerror(errno));
    }
    SystemCallException(std::string sys_call_name, std::string context)
    {
        errormsg = sys_call_name + ": \"" + std::string(strerror(errno)) + "\", context: \"" + context + "\".";
    }
    ~SystemCallException() throw () {}

    const char* what() const throw()
    {
        return errormsg.c_str();
    }
};

/* Create a new socket of type TYPE in domain DOMAIN, using
   protocol PROTOCOL.  If PROTOCOL is zero, one is chosen automatically.
   Returns a file descriptor for the new socket, or -1 for errors.  */
int ws_socket(int __domain, int __type, int __protocol, std::string context)
{
    int socket_fd = socket(__domain, __type, __protocol);
    if (socket_fd == -1)
        throw SystemCallException("socket", context);
    return socket_fd;
}

/* Set socket FD's option OPTNAME at protocol level LEVEL
   to *OPTVAL (which is OPTLEN bytes long).
   Returns 0 on success, -1 for errors.  */
void ws_setsockopt(int __fd, int __level, int __optname, const void *__optval, socklen_t __optlen, std::string context)
{
    if (setsockopt(__fd, __level, __optname, __optval, __optlen) == -1)
        throw SystemCallException("setsockopt", context);
}

/* Give the socket FD the local address ADDR (which is LEN bytes long).  */
void ws_bind(int __fd, __CONST_SOCKADDR_ARG __addr, socklen_t __len, std::string context)
{
    if (bind(__fd, __addr, __len) == -1)
        throw SystemCallException("bind", context);
}

/* Prepare to accept connections on socket FD.
   N connection requests will be queued before further requests are refused.
   Returns 0 on success, -1 for errors.  */
void ws_listen(int __fd, int __n, std::string context)
{
    if (listen(__fd, __n) == -1)
        throw SystemCallException("listen", context);
}

/* Same as epoll_create but with an FLAGS parameter.  The unused SIZE
   parameter has been dropped.  */
int ws_epoll_create1(int __flags, std::string context)
{
    int epoll_fd = epoll_create1(__flags);
    if (epoll_fd == -1)
        throw SystemCallException("epoll_create1", context);
    return epoll_fd;
}

/* Manipulate an epoll instance "epfd". The "op" parameter is one of the EPOLL_CTL_*
   constants defined above. The "fd" parameter is the target of the
   operation. The "event" parameter describes which events the caller
   is interested in and any associated user data.  */
void ws_epoll_ctl(int __epfd, int __op, int __fd, struct epoll_event *__event, std::string context)
{
    if (epoll_ctl(__epfd, __op, __fd, __event) == -1)
        throw SystemCallException("epoll_ctl", context);
}

/* Wait for events on an epoll instance "epfd". Returns the number of
   triggered events returned in "events" buffer.
   The "events" parameter is a buffer that will contain triggered
   events. The "maxevents" is the maximum number of events to be
   returned ( usually size of "events" ). The "timeout" parameter
   specifies the maximum wait time in milliseconds (-1 == infinite).

   This function is a cancellation point and therefore not marked with
   __THROW.  */
int ws_epoll_wait(int __epfd, struct epoll_event *__events, int __maxevents, int __timeout, std::string context)
{
    int nb_events = epoll_wait(__epfd, __events, __maxevents, __timeout);
    if (nb_events == -1)
        throw SystemCallException("epoll_wait", context);
    return nb_events;
}

/* Await a connection on socket FD.
   When a connection arrives, open a new socket to communicate with it,
   set *ADDR (which is *ADDR_LEN bytes long) to the address of the connecting
   peer and *ADDR_LEN to the address's actual length, and return the
   new socket's descriptor, or -1 for errors.

   This function is a cancellation point and therefore not marked with
   __THROW.  */
int ws_accept(int __fd, __SOCKADDR_ARG __addr, socklen_t *__restrict __addr_len, std::string context)
{
    int new_socket = accept(__fd, __addr, __addr_len);
    if (new_socket == -1)
        throw SystemCallException("accept", context);
    return new_socket;
}

/* Close the file descriptor FD.

   This function is a cancellation point and therefore not marked with
   __THROW.  */
void ws_close(int __fd, std::string context)
{
    if (close(__fd) == -1)
    {
        std::cerr << SystemCallException("close", context).what() << std::endl;
    }
}


/* Send N bytes of BUF to socket FD.  Returns the number sent or -1.

   This function is a cancellation point and therefore not marked with
   __THROW.  */
ssize_t ws_send(int __fd, const void *__buf, size_t __n, int __flags, std::string context)
{
    ssize_t nb_sent_bytes = send(__fd, __buf, __n, __flags);
    if (nb_sent_bytes == -1)
        throw SystemCallException("send", context);
    return nb_sent_bytes;
}

/* Wait for events on an epoll instance "epfd". Returns the number of
   triggered events returned in "events" buffer. Or -1 in case of
   error with the "errno" variable set to the specific error code. The
   "events" parameter is a buffer that will contain triggered
   events. The "maxevents" is the maximum number of events to be
   returned ( usually size of "events" ). The "timeout" parameter
   specifies the maximum wait time in milliseconds (-1 == infinite).

   This function is a cancellation point and therefore not marked with
   __THROW.  */
void epoll_wait (int __epfd, struct epoll_event *__events, int __maxevents, int __timeout, std::string context)
{
    if (epoll_wait(__epfd, __events, __maxevents, __timeout) == -1)
        throw SystemCallException("epoll_wait", context);
}

time_t ws_epoch_time_in_mill()
{
    struct timeval time_now;
    if (gettimeofday(&time_now, nullptr) == -1)
        throw SystemCallException("gettimeofday");
    return (time_now.tv_sec * 1000) + (time_now.tv_usec / 1000);
}

ssize_t ws_recv(int socket_fd, char *buffer, size_t buffer_size, int flags)
{
    ssize_t nb_read_bytes;
    if ((nb_read_bytes = recv(socket_fd, buffer, buffer_size, flags)) == -1)
    {
        throw SystemCallException("recv");
    }
    return nb_read_bytes;
}
#endif