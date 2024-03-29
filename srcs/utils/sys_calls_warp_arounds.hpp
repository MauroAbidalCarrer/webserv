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
# include <dirent.h>
# include <fstream>
# include <string>
# include <sstream>
#include <sys/types.h>
#include <sys/stat.h>
class StopWaitLoop {};

# include "WSexception.hpp"



//Exception thrown when a sys call fails.
class SystemCallException : public std::exception
{
    public:
    std::string sys_call_name;
    std::string context;
    std::string error_msg;
    int system_call_errno;
    std::string what_str;

    public:
    SystemCallException(std::string sys_call_name) : 
    sys_call_name(sys_call_name),  context(), error_msg(strerror(errno)), system_call_errno(errno)
    { 
        what_str = "System call \"" + sys_call_name + "\"" + ", strerror: \"" + error_msg + "\"";
    }
    SystemCallException(std::string sys_call_name, std::string context) : 
    sys_call_name(sys_call_name), context(context), error_msg(strerror(errno)), system_call_errno(errno)
    {
        what_str = "System call \"" + sys_call_name + "\"" + ", strerror: \"" + error_msg + "\", context: " + context;
    }
    ~SystemCallException() throw () {}

    const char* what() const throw()
    {
        return what_str.data();
    }
};

/* Create a new socket of type TYPE in domain DOMAIN, using
   protocol PROTOCOL.  If PROTOCOL is zero, one is chosen automatically.
   Returns a file descriptor for the new socket, or -1 for errors.  */
int ws_socket(int __domain, int __type, int __protocol)
{
    int socket_fd = socket(__domain, __type, __protocol);
    if (socket_fd == -1)
        throw SystemCallException("socket");
    return socket_fd;
}

/* Set socket FD's option OPTNAME at protocol level LEVEL
   to *OPTVAL (which is OPTLEN bytes long).
   Returns 0 on success, -1 for errors.  */
void ws_setsockopt(int __fd, int __level, int __optname, const void *__optval, socklen_t __optlen)
{
    if (setsockopt(__fd, __level, __optname, __optval, __optlen) == -1)
        throw SystemCallException("setsockopt");
}

/* Give the socket FD the local address ADDR (which is LEN bytes long).  */
void ws_bind(int __fd, __CONST_SOCKADDR_ARG __addr, socklen_t __len)
{
    if (bind(__fd, __addr, __len) == -1)
        throw SystemCallException("bind");
}

/* Prepare to accept connections on socket FD.
   N connection requests will be queued before further requests are refused.
   Returns 0 on success, -1 for errors.  */
void ws_listen(int __fd, int __n)
{
    if (listen(__fd, __n) == -1)
        throw SystemCallException("listen");
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
void ws_epoll_ctl(int __epfd, int __op, int __fd, struct epoll_event *_event, std::string context)
{
    if (epoll_ctl(__epfd, __op, __fd, _event) == -1)
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
int ws_epoll_wait(int __epfd, struct epoll_event *__events, int __maxevents, int __timeout)
{
    int nb_events = epoll_wait(__epfd, __events, __maxevents, __timeout);
    if (nb_events == -1)
        throw StopWaitLoop();
    return nb_events;
}

/* Await a connection on socket FD.
   When a connection arrives, open a new socket to communicate with it,
   set *ADDR (which is *ADDR_LEN bytes long) to the address of the connecting
   peer and *ADDR_LEN to the address's actual length, and return the
   new socket's descriptor, or -1 for errors.

   This function is a cancellation point and therefore not marked with
   __THROW.  */
int ws_accept(int __fd, __SOCKADDR_ARG __addr, socklen_t *__restrict __addr_len)
{
    int new_socket = accept(__fd, __addr, __addr_len);
    if (new_socket == -1)
        throw SystemCallException("accept");
    return new_socket;
}

/* Close the file descriptor FD.

   This function is a cancellation point and therefore not marked with
   __THROW.  */
void ws_close(int __fd, std::string context)
{
    if (close(__fd) == -1)
        throw SystemCallException("close", context).what();
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
    if (gettimeofday(&time_now, NULL) == -1)
        throw SystemCallException("gettimeofday");
    return (time_now.tv_sec * 1000) + (time_now.tv_usec / 1000);
}

std::string ws_recv(int socket_fd, int buffer_size, int flags, size_t* nb_read_bytes_ptr = NULL)
{
    ssize_t nb_read_bytes;
    char buffer[buffer_size + 1];
    if ((nb_read_bytes = recv(socket_fd, buffer, buffer_size, flags)) == -1)
        throw SystemCallException("recv");
    buffer[nb_read_bytes] = 0;
    if (nb_read_bytes_ptr != NULL)
        *nb_read_bytes_ptr = nb_read_bytes;
    return std::string(buffer, nb_read_bytes);
}

std::string ws_read(int fd, size_t buffer_size, ssize_t* nb_read_bytes_ptr = NULL)
{
    ssize_t nb_read_bytes;
    char buffer[buffer_size + 1];
    if ((nb_read_bytes = read(fd, buffer, buffer_size)) == -1)
        throw SystemCallException("read");
    buffer[nb_read_bytes] = 0;
    if (nb_read_bytes_ptr != NULL)
        *nb_read_bytes_ptr = nb_read_bytes;
    return std::string(buffer, nb_read_bytes);
}

int ws_open(std::string pathname, int flags)
{
    int fd = open(pathname.data(), flags);
    if (fd == -1)
        throw SystemCallException("open" ,"pathname: " + pathname);
    return fd; 
}

void ws_send(int socket_fd, std::string msg, int flags)
{
    if (send(socket_fd, msg.data(), msg.length(), flags) < (ssize_t)msg.length())
        throw SystemCallException("send");
}

size_t get_fstream_size(std::ifstream* stream)
{
    stream->seekg(0, std::ios::end);
    size_t file_size = stream->tellg();
    stream->seekg(0, std::ios::beg);
    return file_size;
}

std::string read_file_into_string(const std::string& filename)
{
    std::ifstream file(filename.data(), std::ios_base::binary);
    if (!file.is_open()) 
    {
        if (errno & ENOENT)
        {
            WSexception e = WSexception("404", SystemCallException("open"));
            // std::cout << "e.response = " << e.response.serialize() << std::endl;
            throw e;
        }
        if (errno & EACCES)
            throw WSexception("406", SystemCallException("open"));
        throw WSexception("500", SystemCallException("open"));
    }
    if (get_fstream_size(&file) > 300000000)
        throw WSexception("500", "Trying to read a file that is bigger that 300MB, size is: ");
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

DIR *ws_opendir(const string& dir_path)
{
    DIR *dir;
    if ((dir = opendir(dir_path.c_str())) == NULL)
    {
        if (errno == EACCES)
            throw WSexception("403", SystemCallException("opendir", "listing directory"));
        if (errno == ENOENT)
            throw WSexception("404", SystemCallException("opendir", "listing directory"));
        throw WSexception("500", SystemCallException("opendir", "listing directory"));
    }
    return dir;
}
std::vector<std::string> list_directory(std::string path_to_directory, int DT_mask)
{
    std::vector<std::string> string_vec;
    DIR *dir;
    struct dirent *ent;
    dir = opendir(path_to_directory.data());
    while ((ent = readdir (dir)) != NULL)
        if (ent->d_type & DT_mask)
            string_vec.push_back(ent->d_name);
    closedir (dir);
    return string_vec;
}
std::vector<std::string> list_files_in_directory(std::string path_to_directory)
{
    return list_directory(path_to_directory, DT_REG);
}
std::vector<std::string> list_files_and_directories_in_directory(std::string path_to_directory)
{
    return list_directory(path_to_directory, DT_REG | DT_DIR);
}

struct stat ws_stat(string filename)
{
    struct stat file_stat;
    if (stat(filename.c_str(), &file_stat) == -1)
    {
        if (errno == EACCES)
            throw WSexception("403", SystemCallException("stat"));
        if (errno == ENOENT)
            throw WSexception("404", SystemCallException("stat"));
        throw WSexception("500", SystemCallException("stat"));
    }
    return file_stat;
}
bool is_regular_file(string filename)
{
    struct stat file_stat = ws_stat(filename);
    return (file_stat.st_mode & S_IFMT) == S_IFREG;
}
bool is_directory(string filename)
{
    struct stat file_stat = ws_stat(filename);
    return (file_stat.st_mode & S_IFMT) == S_IFDIR;
}

void delete_file(string filename)
{
    if (unlink(filename.c_str()) == -1)
    {
        if (errno == EACCES)
            throw WSexception("403", SystemCallException("unlink", "deleting file"));
        if (errno == EBUSY)
            throw WSexception("403", SystemCallException("unlink", "deleting file"));
        if (errno == ENOENT)
            throw WSexception("404", SystemCallException("unlink", "deleting file"));
        throw WSexception("500", SystemCallException("unlink"));
    }
}

bool file_is_ASCII(string filename)
{
    int c;
    std::ifstream a(filename.c_str(), std::ios_base::binary);
    if (a.is_open() == false)
        throw WSexception("404", "Could not open file " + filename);
    while((c = a.get()) != EOF && c <= 127) 
        ;
    return c == EOF;
}
#endif