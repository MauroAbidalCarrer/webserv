#ifndef HTTP_Response_HPP
# define HTTP_Response_HPP
# include <iostream>
# include <string>

# include "HTTP_Message.hpp"

extern std::map<std::string, std::map<std::string, std::string> > CSV_maps;

//sys call functions declarations to avoid circular dependencies (-_-)
std::string read_file_into_string(const std::string& filename);

class HTTP_Response : public HTTP_Message
{
    private:
    bool is_default_response;

    public:
    //constructors and destructors
    HTTP_Response() : is_default_response(false) { }
    HTTP_Response(int read_fd, size_t buffer_size) : HTTP_Message(read_fd, buffer_size), is_default_response(false)
    {
        //do checks
        // status_code = std::atoi(first_line[1].data());
        // status_msg = first_line[2];
    }
    // HTTP_Response(std::string status_code, std::string status_msg)
    // {
    //     first_line.push_back(status_code);
    //     first_line.push_back(status_msg);
    // }
    ~HTTP_Response() { }
    
    //methods
    static HTTP_Response Mk_default_response(std::string status_code)
    {
        HTTP_Response response;
        response.is_default_response = true;
        std::string status_msg = CSV_maps["status_code_to_msg"][status_code];
        response.set_response_line(status_code, status_msg);
        response.body = "<!DOCTYPE html>\n"
                        "<html lang=\"en\">\n"
                        "<head>\n"
                        "\t\t<meta charset=\"UTF-8\">\n"
                        "\t\t<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
                        "\t\t<meta http-equiv=\"X-UA-Compatible\" content=\"ie=edge\">\n"
                        "\t\t<title>";
        response.body.append(status_msg);
        static char second_part[] =    "</title>\n"
                                        "</head>\n"
                                        "<body>\n"
                                        "\t<main>\n"
                                        "\t\t<h1>";
        response.body.append(second_part);
        response.body.append(status_msg);
        static char last_part[] = "</h1>\n"
                                "\t</main>\n"
                                "</body>\n"
                                "</html>\n";
        response.body.append(last_part);
        response.set_header_fields("Content-Type", "text/html;");
        response.set_content_length();
        return response;
    }
    static HTTP_Response mk_from_file_and_status_code(std::string status_code, std::string pathname)
    {
        HTTP_Response response;
        
        //set body content
        // response.body = read_file_content(pathname);
        response.body = read_file_into_string(pathname);
        
        //set content-Type
        response.set_header_fields("Content-Type", "text/plain;");
        //If file has an extension overwrite content-Type. Yes, this is ineffcient, but it's midnight I have to get things done.
        // std::cout << "pathname = " << pathname << std::endl;
        // std::cout << "pathname.find('.') = " << pathname.find('.') << ", std::string::npos = " << std::string::npos << std::endl;
        if (pathname.find('.') != std::string::npos && pathname.find('.') != pathname.length())
        {
            std::string file_extension = pathname.substr(pathname.find('.') + 1, std::string::npos);
            // std::cout << "--------------------------------------found file_extension = " << file_extension << std::endl;
            if (CSV_maps["subtype_to_full_content_type"].count(file_extension))
                response.set_header_fields("Content-Type", CSV_maps["subtype_to_full_content_type"][file_extension]);
        }
        else
            std::cout << "--------------------------------------DID NOT found file_extension = " << std::endl;
        response.set_response_line(status_code, CSV_maps["status_code_to_msg"][status_code]);
        response.set_content_length();
        return response;
    }
    void set_header_fields(std::string header_str, std::string value)
    {
        vector<string> header_fields;
        header_fields.push_back(header_str);
        header_fields.push_back(value);
        for (parsing::tokenized_text_t::iterator i = header.begin(); i != header.end(); i++)
        {
            if ((*i)[0] == header_str)
            {
                *i = header_fields;
                return ;
            }
        }
        header.push_back(header_fields);
    }
    void set_response_line(std::string status_code, std::string status_msg)
    {
        first_line.clear();
        first_line.push_back("HTTP/1.1");
        first_line.push_back(status_code);
        first_line.push_back(status_msg);
    }
    void set_content_length()
    {
        std::ostringstream convert;
        convert << body.length();
        set_header_fields("Content-Length", convert.str());
    }
    virtual string debug()
    {
        if (is_default_response)
            return "Default " + first_line[1] + " response.\n";
        return HTTP_Message::debug();
    }
};
#endif