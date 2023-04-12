#ifndef HTTP_Response_HPP
# define HTTP_Response_HPP
# include <iostream>
# include <string>
# include <dirent.h>

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
    HTTP_Response() : HTTP_Message(), is_default_response(false) { }
    ~HTTP_Response() { }
    //Construct respoonse for directory listing
    HTTP_Response(string directory_path)
    {
        is_default_response = true;
        set_response_line("200", CSV_maps["status_code_to_msg"]["200"]);
        body = "<!DOCTYPE html>\n"
                        "<html lang=\"en\">\n"
                        "<head>\n"
                        "\t\t<meta charset=\"UTF-8\">\n"
                        "\t\t<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
                        "\t\t<meta http-equiv=\"X-UA-Compatible\" content=\"ie=edge\">\n"
                        "\t\t<title>";
        body.append(directory_path);
        static char second_part[] =    "</title>\n"
                                        "</head>\n"
                                        "<body>\n"
                                        "\t<main>\n"
                                        "\t\t<p>\n";
        body.append(second_part);
        DIR *dir;
        struct dirent *ent;
        if ((dir = opendir(directory_path.data())) != NULL)
        {
            while ((ent = readdir (dir)) != NULL)
            {
                if (ent->d_type & (DT_REG | DT_DIR))
                {
                    string element_name = ent->d_name;
                    if (element_name == ".")
                        continue;;
                    if (ent->d_type & DT_DIR)
                        element_name += "/";
                    body.append("\t\t\t<a href=\"");
                    body.append(element_name);
                    body.append("\">");
                    body.append(element_name);
                    body.append("</a><br>");
                }
            }
            closedir (dir);
        }
        else
            throw runtime_error("Failed to list directory, opendir failed.");
        static char last_part[] =   "\t\t</p>"
                                    "\t</main>\n"
                                    "</body>\n"
                                    "</html>\n";
        body.append(last_part);
        set_header_fields("Content-Type", "text/html;");
        set_content_length();

    }
    
    //methods
    void partial_constructor_from_fd(int read_fd)
    {
        HTTP_Message::partial_constructor(read_fd);
        if (is_fully_constructed)
        {
            
        }
    }
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
    static HTTP_Response mk_from_regualr_file_and_status_code(std::string status_code, std::string pathname)
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
		response.set_header_fields("Connection", "keep-alive");
        return response;
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
            return "\e[2mDefault " + first_line[1] + " response.\n\e[0m";
        return HTTP_Message::debug();
    }
    string get_status_code() const
    {
        if (first_line.size() >= 2)
            return first_line[1];
        return "";
    }
    bool is_error() const
    {
		string response_status_code = get_status_code();
		return (response_status_code[0] == '5' || response_status_code[0] == '4');
    }
};
#endif