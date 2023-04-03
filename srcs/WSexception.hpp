#ifndef WSexception_HPP
# define WSexception_HPP
# include <iostream>
# include <string>

# include "HTTP_Response.hpp"

class WSexception : public std::exception
{
    public:
    std::exception * exception_ptr;
    HTTP_Response response;

    public:
    WSexception(std::string status_code) 
    {
        response = HTTP_Response::Mk_default_response(status_code);
        exception_ptr = NULL;
    }
    WSexception(std::string status_code, const exception& exception)
    {
        response = HTTP_Response::Mk_default_response(status_code);
        exception_ptr = new std::exception(exception);
    }
    ~WSexception() throw()
    {
        if (exception_ptr != NULL)
            delete exception_ptr;
    }
    const char* what() const throw()
    {
        if (exception_ptr != NULL)
            return exception_ptr->what();
        return "ERROR: exception_ptr of WSexception is NULL";
    }
};
#endif