#ifndef WSexception_HPP
# define WSexception_HPP
# include <iostream>
# include <string>

class WSexception : std::exception
{
    private:
    std::string error_msg;

    public:
    //constructors and destructors
    WSexception() { }
    WSexception(std::string error_msg) : error_msg(error_msg) { }
    ~WSexception() throw() { }
    const char * what() const throw()
    {
        return error_msg.data();
    }
};
// class SystemCallException : public std::exception
// {
//     private:
//     std::string errormsg;

//     public:
//     SystemCallException(std::string sys_call_name)
//     {
//         errormsg = sys_call_name + ": " + std::string(strerror(errno));
//     }
//     SystemCallException(std::string sys_call_name, std::string context)
//     {
//         errormsg = sys_call_name + ": \"" + std::string(strerror(errno)) + "\", context: \"" + context + "\".";
//     }
//     ~SystemCallException() throw () {}

//     const char* what() const throw()
//     {
//         return errormsg.c_str();
//     }
// };
#endif