#include "Server.hpp"
#include <iostream>
#include <map>
#include <string>
#include <cstring>

// int main()
// {
//     try
//     {
//         Server server;
//         server.Run();
//     }
//     catch(const std::exception& e)
//     {
//         std::cerr << e.what() << '\n';
//     }
// }   


// # include "parsing.hpp"
// # include "HTTP_Request.hpp"
// int main()
// {
//     const char *http_get_request = "GET / HTTP/1.1\r\n"
//                                "Host: www.example.com\r\n"
//                                "Connection: close\r\n"
//                                "\r\n";

//     // HTTP_Request()
//     int GET_example_file_fd = ws_open("GET_example", O_WRONLY | O_CREAT);
//     write(GET_example_file_fd, http_get_request, std::string(http_get_request).length());
// }

int main()
{
    int GET_example_file_fd = ws_open("GET_example", O_RDONLY);
    // std::string str = ws_read(GET_example_file_fd, 60);
    // std::cout << str << std::endl;
    std::cout << " GET_example_file_fd = " << GET_example_file_fd << std::endl;
    HTTP_Request request(GET_example_file_fd, 60);
    std::cout << std::endl;
    for (size_t i = 0; i < request.header.size(); i++)
        for (size_t j = 0; j < request.header[i].size(); j++)
            std::cout << '-' << request.header[i][j] << std::endl;
}