#include "Server.hpp"
#include <iostream>
#include <map>
#include <string>
#include <cstring>

std::map<std::string, std::map<std::string, std::string> > CSV_maps;

int main()
{
    try
    {
        Server server;
        server.Run();
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
}   

// #include "sys_calls_warp_arounds.hpp"
// #include "parsing.hpp"
// typedef std::vector<std::string> string_vec_t;
// #include <map>
// #include <string>
// typedef std::map<std::string, std::string> str_to_str_map_t;
// int main(int ac, char **av)
// {
//     if (ac < 2)
//     {
//         std::cerr << "ac = " << ac << std::endl;
//         return 1;
//     }
//     try
//     {
//         string_vec_t MIME_type_to_subtype_file_names = list_files_in_directory(av[1]);
//         str_to_str_map_t subtype_to_full_content_type;
//         for (size_t i = 0; i < MIME_type_to_subtype_file_names.size(); i++)
//         {
//             std::string MIME_type_to_subtype_file_name_path = av[1] + MIME_type_to_subtype_file_names[i];
//             std::cout << MIME_type_to_subtype_file_name_path << std::endl;
//             std::string file_content = read_file_content(MIME_type_to_subtype_file_name_path);
//             parsing::tokenized_text_t tokenized_file_content = parsing::tokenize_text(file_content, "\n", ", ");
//             std::cout << tokenized_file_content.size() << std::endl;
//             for (size_t j = 0; j < tokenized_file_content.size(); j++)
//             {
//                 string_vec_t line = tokenized_file_content[j];
//                 if (line.size() >= 2 && !line[0].empty() && !line[1].empty() && line[0] != "Name")
//                 {
//                     std::cout << "adding key = " << line[0] << ", value = " << line[1] << std::endl;
//                     subtype_to_full_content_type[line[0]] = line[1];
//                 }
//             }
//         }
//         //create final file
//         std::string subtype_to_full_content_type_pathname = "subtype_to_full_content_type.csv";
//         std::cout << "subtype_to_full_content_type_pathname = " << subtype_to_full_content_type_pathname << std::endl;
//         int subtype_to_full_content_type_fd = ws_open(subtype_to_full_content_type_pathname, O_CREAT | O_TRUNC | O_WRONLY);
//         for (str_to_str_map_t::iterator i = subtype_to_full_content_type.begin(); i != subtype_to_full_content_type.end(); i++)
//         {
//             write(subtype_to_full_content_type_fd, i->first.data(), i->first.length());
//             write(subtype_to_full_content_type_fd, ",", 1);
//             write(subtype_to_full_content_type_fd, i->second.data(), i->second.length());
//             write(subtype_to_full_content_type_fd, "\n", 1);
//         }
//         ws_close(subtype_to_full_content_type_fd, "closing subtype_to_full_content_type_fd");
//     }
//     catch(std::exception& e)
//     {
//         std::cerr << "Caught exception: " << e.what() << std::endl;
//     }
// }