#ifndef IO_Manager_HPP
# define IO_Manager_HPP
# include <iostream>
# include "sys_calls_warp_arounds.hpp"
# include <string>
# include <map>
# include <vector>

# define MAX_EPOLL_EVENTS_TO_HANDLE_AT_ONCE 64

typedef void (*fd_callback_t)(int);
enum timeout_mode_e
{
    renew_after_IO_operation,//for connections, could also be no_timeout for connections
    do_not_renew,//for CGIs
    no_timeout//for listening sockets
};



class IO_Manager
{
    private:
    //nested classes
    class Duplicate_IO_Manager_Exception : public std::exception
    {
        const char* what() const throw()
        {
            return "Error: Duplicate_IO_Manager_Exception: there should only be one IO_Manager instance in the process!";
        }
    };
    
    class IO_callbacks
    {
        fd_callback_t read_callback;
        fd_callback_t write_callback;
        fd_callback_t timeout_callback;
        protected:
        int fd;
        timeout_mode_e timeout_mode; 
        clock_t timeout;
        clock_t last_io;
        public:
        IO_callbacks(int timeout, timeout_mode_e timeout_mode, int fd) :
        read_callback(NULL), write_callback(NULL), timeout_callback(NULL), fd(fd), timeout(timeout), timeout_mode(timeout_mode), last_io(ws_clock())
        { }
        public:
        IO_callbacks() : 
        read_callback(NULL), write_callback(NULL), timeout_callback(NULL), fd(-1), timeout(0), timeout_mode(), last_io(ws_clock())
        { 
            std::cerr << "Warning: instanciating IO_callbacks with empty constructor." << std::endl;
        }
        IO_callbacks(fd_callback_t read_callback, fd_callback_t write_callback, fd_callback_t timeout_callback, clock_t timeout, timeout_mode_e timeout_mode, int fd) : 
        read_callback(read_callback), 
        write_callback(write_callback), 
        timeout_callback(timeout_callback),
        fd(fd),
        timeout(timeout),
        last_io(ws_clock()),
        timeout_mode(timeout_mode)
        { }
        ~IO_callbacks() {}
        virtual void call_event_callbacks(epoll_event event, clock_t current_time)
        {
            if ((event.events | EPOLLIN) && read_callback)
            {
                if (timeout_mode == renew_after_IO_operation)
                    last_io = current_time;
                (*read_callback)(event.data.fd);
            }
            if ((event.events | EPOLLOUT) && write_callback)
            {
                if (timeout_mode == renew_after_IO_operation)
                    last_io = current_time;
                (*write_callback)(event.data.fd);
            }
        }
        virtual void call_timeout_callback()
        {
            if (timeout_callback)
                (*timeout_callback)(fd);
        }
        clock_t get_timeout_time()
        {
            if (timeout_mode == no_timeout)
               return -1;
            return last_io + timeout;
        }
    };
    template<typename T>class TemplateIO_callbacks : public IO_callbacks
    {
        void (*read_callback)(T*, int);
        void (*write_callback)(T*, int);
        void (*timeout_callback)(T*, int);
        T* instance;
        public:
        TemplateIO_callbacks(void (*read_callback)(T*, int), void (*write_callback)(T*, int), void (*timeout_callback)(T*, int), T* instance, int timeout, timeout_mode_e timeout_mode, int fd) :
        read_callback(read_callback),
        write_callback(write_callback), 
        timeout_callback(timeout_callback), 
        instance(instance),
        IO_callbacks(timeout, timeout_mode, fd)
        { }
        ~TemplateIO_callbacks() {}
        void call_event_callbacks(epoll_event event)
        {
            if ((event.events | EPOLLIN) && read_callback)
                    (*read_callback)(instance, event.data.fd);
            if ((event.events | EPOLLOUT) && write_callback)
                    (*write_callback)(instance, event.data.fd);
        }
        void call_timeout_callback(int fd)
        {
            if (timeout_callback)
                (*timeout_callback)(instance, fd);
        }
    };
  
    
    //private fields
    int epoll_fd;
    typedef std::map<int, IO_callbacks>::iterator interest_map_it_t;
    std::map<int, IO_callbacks> interest_map;
    std::vector<int> fds_to_close;

    //public fields
    //constructors and destructors
    IO_Manager() : interest_map(), fds_to_close()
    {
        epoll_fd = ws_epoll_create1(EPOLL_CLOEXEC, "creating epoll instance for IO_Manager");
    }
    ~IO_Manager()
    {
        ws_close(epoll_fd, "closing epoll_fd in IO_Manager destructor");
    }

    //methods
    public:
    static IO_Manager& singleton()
    {
        static IO_Manager singleton_instance;
        return singleton_instance;
    }
    //Adds IO_callback if it doesn't already exists, or modifies it otherwise.
    void set_interest(int fd, epoll_event event, IO_callbacks callbacks)
    {
        //interest_map.count(fd) allows us to check if the map contains the fd
        int op = interest_map.count(fd) ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
        interest_map[fd] = callbacks;
        epoll_ctl(epoll_fd, op, fd, &event);
    }
    static void set_interest(int fd, fd_callback_t read_callback, fd_callback_t write_callback, fd_callback_t timeout_calback, int timeout, timeout_mode_e timeout_mode)
    {
        epoll_event event;
        event.data.fd = fd;
        event.events = 0;
        if (read_callback)
            event.events |= EPOLLIN;
        if (write_callback)
            event.events |= EPOLLOUT;
        singleton().set_interest(fd, event, IO_callbacks(read_callback, write_callback, timeout_calback, timeout, timeout_mode, fd));
    }
    template <typename T> static void set_interest(int fd, void (*read_callback)(T*, int), void (*write_callback)(T*, int), void (*timeout_callback)(T*, int), int timeout, timeout_mode_e timeout_mode)
    {
        epoll_event event;
        event.data.fd = fd;
        event.events = 0;
        if (read_callback)
            event.events |= EPOLLIN;
        if (write_callback)
            event.events |= EPOLLOUT;
        singleton().set_interest(fd, event, TemplateIO_callbacks<T>(read_callback, write_callback, timeout_callback, timeout, timeout_mode));
    }
    void remove_interest_and_close_fd(int fd)
    {
        fds_to_close.push_back(fd);
    }
    
    void wait_and_call_callbacks()
    {
        int current_time = ws_clock();
        static epoll_event events[MAX_EPOLL_EVENTS_TO_HANDLE_AT_ONCE];
        int nb_events = ws_epoll_wait(epoll_fd, events, MAX_EPOLL_EVENTS_TO_HANDLE_AT_ONCE, get_timeout_for_epoll_wait(current_time), "IO_manager wait loop");
        //timeout
        if (nb_events == 0)
        {
            for (interest_map_it_t i = interest_map.begin(); i != interest_map.end(); i++)
            {
                clock_t interest_timeout = i->second.get_timeout_time();
                if (interest_timeout != -1 && interest_timeout <= current_time)
                    i->second.call_timeout_callback();
            }
        }
        //event happend
        else for (int i = 0; i < nb_events; i++)
        {
            epoll_event event = events[i];
            interest_map[event.data.fd].call_event_callbacks(event, current_time);
        }
        for (size_t i = 0; i < fds_to_close.size(); i++)
        {
            int fd = fds_to_close[i];
            ws_epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL, "removing fd from epoll instance in \"remove_interest_and_close_fd\"");
            ws_close(fd, "closing fd before removing it from interest_map");
            interest_map.erase(fd);
        }
        fds_to_close.clear();
    }
    clock_t get_timeout_for_epoll_wait(int current_clock_time)
    {
        interest_map_it_t i = interest_map.begin();
        clock_t timeout = i->second.get_timeout_time();
        for (; i != interest_map.end(); i++)
        {
            clock_t other_timeout = i->second.get_timeout_time();
            if (other_timeout != -1 && other_timeout > timeout)
                timeout = other_timeout- current_clock_time;
        }
        return timeout;
    }
};
#endif