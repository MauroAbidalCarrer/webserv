#ifndef WSexception_HPP
# define WSexception_HPP
# include <iostream>
# include <string>

# include "HTTP_Response.hpp"
# include "typedefs.hpp"


class HTTP_Response;

class WSexception : public std::exception
{
    public:
    // std::exception * exception_ptr;
    std::string exception_what;
    HTTP_Response response;

    public:
    WSexception(const std::string& status_code) 
    {
        response = HTTP_Response::Mk_default_response(status_code);
        // exception_ptr = NULL;
        exception_what = "WSexception created with satus_code \"" + status_code + " " + CSV_maps["status_code_to_msg"][status_code] + "\"";
    }
    WSexception()
    {
        response = HTTP_Response::Mk_default_response("500");
        // exception_ptr = NULL;
        exception_what = "WSexception created with satus_code \"500 " + CSV_maps["status_code_to_msg"]["500"] + "\"";
    }
    WSexception(std::string status_code, const exception& exception)
    {
        response = HTTP_Response::Mk_default_response(status_code);
        std::cerr << "WSexception constructed with default HTTP_response for status code " << status_code << std::endl;
        exception_what = exception.what();
    }
    WSexception(const string& status_code, const string& what_msg)
    {
        response = HTTP_Response::Mk_default_response(status_code);
        exception_what = what_msg;
    }
    WSexception(const WSexception& other)
    {
        *this = other;
    }
    //operator overloads
    WSexception& operator=(const WSexception& rhs)
    {
        response = rhs.response;
        // exception_ptr = rhs.exception_ptr;
        exception_what = rhs.exception_what;
        // std::cout << "copied WS_exception, response = " << std::endl << response.serialize();
        return *this;
    }
    ~WSexception() throw()
    {
        // if (exception_ptr != NULL)
        //     delete exception_ptr;
    }

    //methods
    
    const char* what() const throw()
    {
        // if (exception_ptr != NULL)
        //     return exception_ptr->what();
        // return "ERROR: exception_ptr of WSexception is NULL";
        return exception_what.data();
    }
};
void throw_WSexcetpion(const string& status_code, const string& what_msg)
{
    throw WSexception(status_code, what_msg);
}
void throw_WSexcetpion(const string& status_code)
{
    throw WSexception(status_code);
}
#endif