#ifndef IO_Manager_HPP
# define IO_Manager_HPP
# include <iostream>
# include <string>
# include <map>
# include <vector>

# include "sys_calls_warp_arounds.hpp"

# define CLOCKS_PER_MILLISECONDS CLOCKS_PER_SEC / 1000
# define MAX_EPOLL_EVENTS_TO_HANDLE_AT_ONCE 64

typedef void (*static_fd_callback_t)(int);
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
    
    public:
    class FD_interest
    {
        //fields
        protected:
        int fd;
        timeout_mode_e timeout_mode; 
        time_t timeout_in_mill;
        time_t last_io_in_mill;

        //constructors
        public:
        FD_interest(time_t timeout_in_mill, timeout_mode_e timeout_mode, int fd) :
        fd(fd), timeout_mode(timeout_mode), timeout_in_mill(timeout_in_mill), last_io_in_mill(ws_epoch_time_in_mill())
        { }
        public:
        FD_interest() : //This constructor is required by map.
        fd(-1), timeout_mode(), timeout_in_mill(0), last_io_in_mill(ws_epoch_time_in_mill())
        { }
        virtual ~FD_interest() {};

        //methods
        void call_event_callbacks(epoll_event event, time_t current_time_in_mill)
        {
            if (timeout_mode == renew_after_IO_operation)
                last_io_in_mill = current_time_in_mill;
            internal_call_event_callbacks(event);
        }
        virtual void internal_call_event_callbacks(epoll_event event) = 0;
        virtual void call_timeout_callback() = 0;
        time_t get_timeout_in_mill()
        {
            if (timeout_mode == no_timeout)
               return -1;
            return last_io_in_mill + timeout_in_mill;
        }
    };
    //fd intrest with static function callbacks
    class static_FD_interest : public FD_interest
    {
        //fields
        static_fd_callback_t read_callback;
        static_fd_callback_t write_callback;
        static_fd_callback_t timeout_callback;
        
        //constructors
        public:
        static_FD_interest(static_fd_callback_t read_callback, static_fd_callback_t write_callback, static_fd_callback_t timeout_callback, time_t timeout_in_mill, timeout_mode_e timeout_mode, int fd) : 
        FD_interest(timeout_in_mill, timeout_mode, fd),
        read_callback(read_callback), 
        write_callback(write_callback), 
        timeout_callback(timeout_callback)
        { }
        ~static_FD_interest(){}
        
        //methods
        void internal_call_event_callbacks(epoll_event event)
        {
            if ((event.events & EPOLLIN) && read_callback)
                (*read_callback)(event.data.fd);
            if ((event.events & EPOLLOUT) && write_callback)
                (*write_callback)(event.data.fd);
        }
        void call_timeout_callback()
        {
            if (timeout_callback)
                (*timeout_callback)(fd);
        }
    };
    //fd_interest with member funciton callbacks
    template<typename T>class Template_IO_interest : public FD_interest
    {
        //fields
        void (T::*read_callback)(int);
        void (T::*write_callback)(int);
        void (T::*timeout_callback)(int);
        T* instance;

        //methods
        public:
        Template_IO_interest(void (T::*read_callback)(int), void (T::*write_callback)(int), void (T::*timeout_callback)(int), time_t timeout_in_mill, timeout_mode_e timeout_mode, int fd, T* instance) :
        FD_interest(timeout_in_mill, timeout_mode, fd),
        read_callback(read_callback),
        write_callback(write_callback), 
        timeout_callback(timeout_callback), 
        instance(instance)
        { }
        ~Template_IO_interest(){}

        //methods
        void internal_call_event_callbacks(epoll_event event)
        {
            if ((event.events & EPOLLIN) && read_callback)
                (instance->*read_callback)(event.data.fd);
            if ((event.events & EPOLLOUT) && write_callback)
                (instance->*write_callback)(event.data.fd);
        }
        void call_timeout_callback()
        {
            if (timeout_callback)
                (instance->*timeout_callback)(fd);
        }
    };
  
    
    //private fields
    private:
    time_t current_time_in_mill;
    typedef std::map<int, FD_interest * >::iterator interest_map_it_t;
    std::map<int, FD_interest *> interest_map;
    std::vector<int> fds_to_close;

    //constructors and destructors
    IO_Manager() : interest_map(), fds_to_close()
    {
        epoll_fd = ws_epoll_create1(EPOLL_CLOEXEC, "Creating epoll instance for IO_Manager.");
    }
    ~IO_Manager()
    {
        clear_all_fds();
        ws_close(epoll_fd, "Closing epoll_fd in IO_Manager destructor.");
    }

    //methods
    private:
    static IO_Manager& singleton()
    {
        static IO_Manager singleton_instance;
        return singleton_instance;
    }
    public:
    //Adds IO_callback if the key(fd) doesn't already exists, or modifies it otherwise.
    void non_static_set_interest(int fd, int epoll_event_mask, FD_interest * FD_interst_ptr)
    {
        int op = EPOLL_CTL_ADD;
        //interest_map.count(fd) allows us to check if the map contains the fd
        if (interest_map.count(fd))
        {
            op = EPOLL_CTL_MOD;
            delete interest_map[fd];
        }
        epoll_event event;
        event.events = epoll_event_mask;
        event.data.fd = fd;
        interest_map[fd] = FD_interst_ptr;
        ws_epoll_ctl(epoll_fd, op, fd, &event, "setting fd interest");
    }
    public:
    class StopWaitLoop {};
    int epoll_fd;
    //static overload for static functions
    static void set_interest(int fd, int epoll_event_mask, FD_interest * FD_interst_ptr)
    {
        singleton().non_static_set_interest(fd, epoll_event_mask, FD_interst_ptr);
    }
    static void set_interest(int fd, static_fd_callback_t read_callback, static_fd_callback_t write_callback)
    {
        set_interest(fd, read_callback, write_callback, NULL, -1, no_timeout);
    }
    static void set_interest(int fd, static_fd_callback_t read_callback, static_fd_callback_t write_callback, static_fd_callback_t timeout_calback, time_t timeout_in_mill, timeout_mode_e timeout_mode)
    {
        int epoll_event_mask = 0;
        if (read_callback)
            epoll_event_mask |= EPOLLIN;
        if (write_callback)
            epoll_event_mask |= EPOLLOUT;
        FD_interest * callbacks_ptr = new static_FD_interest(read_callback, write_callback, timeout_calback, timeout_in_mill, timeout_mode, fd);
        singleton().non_static_set_interest(fd, epoll_event_mask, callbacks_ptr);
    }
    //static overload of set_interest
    template <typename T> static void set_interest(int fd, void (T::*read_callback)(int), void (T::*write_callback)(int), T* instance)
    {
        set_interest<T>(fd, read_callback, write_callback, NULL, -1, no_timeout, instance);
    }
    template <typename T> static void set_interest(int fd, void (T::*read_callback)(int), void (T::*write_callback)(int), void (T::*timeout_callback)(int), time_t timeout_in_mill, timeout_mode_e timeout_mode, T* instance)
    {
        int epoll_event_mask = 0;
        if (read_callback)
            epoll_event_mask |= EPOLLIN;
        if (write_callback)
            epoll_event_mask |= EPOLLOUT;
        FD_interest * callbacks_ptr = new Template_IO_interest<T>(read_callback, write_callback, timeout_callback, timeout_in_mill, timeout_mode, fd, instance);
        singleton().non_static_set_interest(fd, epoll_event_mask, callbacks_ptr);
    }
    //change interest epoll_mask
    void non_static_change_interest_epoll_mask(int fd, int epoll_event_mask)
    {
        epoll_event event;
        event.events = epoll_event_mask;
        event.data.fd = fd;
        ws_epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &event, "changing FD interest");
        std::cout << "                    set fd  " << fd << " event.events = " << event.events << std::endl;
    }
    static void change_interest_epoll_mask(int fd, int epoll_event_mask)
    {
        singleton().non_static_change_interest_epoll_mask(fd, epoll_event_mask);
    }
    //remove
    void non_static_remove_interest_and_close_fd(int fd)
    {
        fds_to_close.push_back(fd);
    }
    static void remove_interest_and_close_fd(int fd)
    {
        singleton().non_static_remove_interest_and_close_fd(fd);
    }
    
    void non_static_wait_and_call_callbacks()
    {
        if (interest_map.size() == 0)
        {
            cout << "Warning:calling IO_Manager::" << __func__ << " with zero fd interests." << endl;
            cout << "\tNot starting the epoll_wait lopp to avoid endless loop." << endl;
            return ;
        }
        try
        {
            do
            {
                current_time_in_mill = ws_epoch_time_in_mill();
                static epoll_event events[MAX_EPOLL_EVENTS_TO_HANDLE_AT_ONCE];
                int sortest_timeout_in_mill = find_shortest_timeout_in_milliseconds();
                int nb_events = ws_epoll_wait(epoll_fd, events, MAX_EPOLL_EVENTS_TO_HANDLE_AT_ONCE, sortest_timeout_in_mill, "IO_manager wait loop");
                current_time_in_mill = ws_epoch_time_in_mill();
                //timeout
                if (nb_events == 0)
                    handle_timeout();
                //event happend
                else for (int i = 0; i < nb_events; i++)
                {
                    epoll_event event = events[i];
                    interest_map[event.data.fd]->call_event_callbacks(event, current_time_in_mill);
                }
                clear_fds_to_close();
            } while (true);
        }
        catch(StopWaitLoop& e)
        {
            std::cout << "Stopped epoll_wait loop." << std::endl;
        }
    }
    static void wait_and_call_callbacks()
    {
        singleton().non_static_wait_and_call_callbacks();
    }
    int find_shortest_timeout_in_milliseconds()
    {
        interest_map_it_t i = interest_map.begin();
        time_t timeout = i->second->get_timeout_in_mill();
        for (; i != interest_map.end(); i++)
        {
            time_t other_timeout = i->second->get_timeout_in_mill();
            if (other_timeout != -1 && other_timeout > timeout)
                timeout = other_timeout - current_time_in_mill;
        }
        if (timeout != -1)
            std::cout << "shortest timeout in milliseconds = " << timeout << std::endl << std::endl;
        else
            std::cout << "shortest timeout in milliseconds = NO TIMEOUT" << std::endl << std::endl;
        return timeout;
    }
    void handle_timeout()
    {
        for (interest_map_it_t i = interest_map.begin(); i != interest_map.end(); i++)
        {
            time_t interest_timeout_in_mill = i->second->get_timeout_in_mill();
            if (interest_timeout_in_mill != -1 && interest_timeout_in_mill <= current_time_in_mill)
                i->second->call_timeout_callback();
        }
    }
    void clear_fds_to_close()
    {
        for (size_t i = 0; i < fds_to_close.size(); i++)
        {
            int fd = fds_to_close[i];
            ws_epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL, "removing fd from epoll instance in \"remove_interest_and_close_fd\"");
            ws_close(fd, "closing fd before removing it from interest_map");
            delete interest_map[fd];
            interest_map.erase(fd);
        }
        fds_to_close.clear();
    }
    void clear_all_fds()
    {
        for (interest_map_it_t i = interest_map.begin(); i != interest_map.end(); i++)
        {
            ws_close(i->first, "closing fd at end of loop");
            delete i->second;
        }
    }
};
#endif