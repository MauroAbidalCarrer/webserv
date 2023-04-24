#ifndef HTTP_Response_HPP
# define HTTP_Response_HPP
# include <iostream>
# include <string>
# include <dirent.h>
# include <algorithm>

# include "HTTP_Message.hpp"
# include "LocationContext.hpp"

extern std::map<std::string, std::map<std::string, std::string> > CSV_maps;

//sys call functions declarations to avoid circular dependencies (-_-)
std::string read_file_into_string(const std::string& filename);
time_t ws_epoch_time_in_mill();

class HTTP_Response : public HTTP_Message
{
    private:
    bool is_default_response;

    public:
    //constructors and destructors
    HTTP_Response() : HTTP_Message(), is_default_response(false) { }
    ~HTTP_Response() { }
    //Construct respoonse for directory listing
    static void set_directory_listing_response(HTTP_Response& response_dst, const string& request_path, const LocationContext& locationContext)
    {
        response_dst.is_default_response = true;
        response_dst.set_response_line("200", CSV_maps["status_code_to_msg"]["200"]);
        response_dst.body = "<!DOCTYPE html>\n"
                        "<html lang=\"en\">\n"
                        "<style>"
                        "table {"
                        "font-family: arial, sans-serif;"
                        "border-collapse: collapse;"
                        "width: 100%;"
                        "}"
                        ""
                        "td, th {"
                        "border: 1px solid #dddddd;"
                        "text-align: left;"
                        "padding: 8px;"
                        "}"
                        ""
                        "tr:nth-child(even) {"
                        "background-color: #dddddd;"
                        "}"
                        "</style>"
                        "<head>\n"
                        "\t\t<meta charset=\"UTF-8\">\n"
                        "\t\t<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
                        "\t\t<meta http-equiv=\"X-UA-Compatible\" content=\"ie=edge\">\n"
                        "\t\t<title>";
        response_dst.body.append(request_path);
        static char second_part[] =    "</title>\n"
                                        "</head>\n"
                                        "<body>\n"
                                        "\t<main>\n"
                                        "\t\t<table>\n"
                                        "\t\t\t<tr><th>Name</th><th>Size</th></tr>";
        response_dst.body.append(second_part);
        DIR *dir;
        struct dirent *ent;
        if ((dir = opendir(request_path.data())) != NULL)
        {
            while ((ent = readdir (dir)) != NULL)
            {
                if (ent->d_type & (DT_REG | DT_DIR))
                {
                    string element_name = ent->d_name;
                    if (element_name == ".")
                        continue;
                    if (ent->d_type & DT_DIR)
                        element_name += "/";
                    response_dst.body.append("\t\t\t<tr><th><a href=\"");
                    response_dst.body.append(locationContext.path);
                    if (locationContext.path[locationContext.path.size() - 1] != '/')
                        response_dst.body.append("/");
                    response_dst.body.append(element_name);
                    response_dst.body.append("\">");
                    response_dst.body.append(element_name);
                    response_dst.body.append("</a></th><th>");
                    std::ostringstream convert;
                    convert << ent->d_reclen;
                    response_dst.body.append(convert.str());
                    response_dst.body.append("</th></tr>\n");
                }
            }
            closedir (dir);
        }
        else
            throw runtime_error("Failed to list directory, opendir failed.");
        if (std::find(locationContext.allowed_methods.begin(), locationContext.allowed_methods.end(), "DELETE") != locationContext.allowed_methods.end())
        {
            response_dst.body.append(
                                "\t\t</table>"
                                "<form id=\"delete-form\" method=\"POST\">"
                                "<label for=\"url-input\">URL:</label>"
                                "<input type=\"text\" id=\"url-input\" name=\"url\" value=\"/your/api/endpoint\">"
                                "<!-- add any other form fields you need here -->"
                                "<button type=\"submit\">Delete</button>"
                                "</form>"
                                ""
                                "<div id=\"message\"></div>"
                                ""
                                "<script>"
                                "const form = document.getElementById('delete-form');"
                                "const message = document.getElementById('message');"
                                "form.addEventListener('submit', (event) => {"
                                "event.preventDefault();"
                                "const xhr = new XMLHttpRequest();"
                                "const url = document.getElementById('url-input').value;"
                                "xhr.open('DELETE', url);"
                                "xhr.onload = function() {"
                                "if (xhr.status === 200) {"
                                "message.textContent = 'Deleted successfully!';"
                                "} else {"
                                "message.textContent = 'Error deleting: ' + xhr.statusText;"
                                "}"
                                "};"
                                "xhr.send(new FormData(form));"
                                "});"
                                "</script>"
                                );
        }
        response_dst.body.append("\t</main>\n"
                                 "</body>\n"
                                 "</html>\n");
        response_dst.set_header_fields("Content-Type", "text/html;");
        response_dst.set_content_length();
    }
    
    //methods
    void construct_from_CGI_output(int read_fd)
    {
        ssize_t nb_read_bytes = 0;
        if (header_is_constructed == false)
            construct_header(read_fd);
        else
            body.append(ws_read(read_fd, READ_BUFFER_SIZE, &nb_read_bytes));
        is_fully_constructed = nb_read_bytes < READ_BUFFER_SIZE;
    }
    ssize_t construct_header(int read_fd)
    {
        ssize_t nb_read_bytes = HTTP_Message::construct_header(read_fd, "500", "500");
        body.insert(body.begin(), construct_buffer.begin() + construct_buffer.find("\r\n\r\n") + 4, construct_buffer.end());
        header_is_constructed = true;
        PRINT("In Header construcion:");
        PRINT("Construction of HTTP message expects EOF as end of message, nb_read_bytes: " << nb_read_bytes << ", READ_BUFFER_SIZE: " << READ_BUFFER_SIZE << ", nb_read_bytes < READ_BUFFER_SIZE: " << (nb_read_bytes < READ_BUFFER_SIZE));
        return nb_read_bytes;
    }
    static void set_redirection_response(HTTP_Response& response_dst, string new_url)
    {
        response_dst = HTTP_Response::Mk_default_response("301");
        response_dst.set_header_fields("Location", new_url);
        response_dst.set_header_fields("Connection", "keep-alive");
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
        response.body = read_file_into_string(pathname);

        //set content-Type
        response.set_header_fields("Content-Type", "text/plain;");
        //If file has an extension overwrite content-Type. Yes, this is ineffcient, but it's midnight I have to get things done.
        if (pathname.find('.') != std::string::npos && pathname.find('.') != pathname.length())
        {
            std::string file_extension = pathname.substr(pathname.find('.') + 1, std::string::npos);
            if (CSV_maps["subtype_to_full_content_type"].count(file_extension))
                response.set_header_fields("Content-Type", CSV_maps["subtype_to_full_content_type"][file_extension]);
        }
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