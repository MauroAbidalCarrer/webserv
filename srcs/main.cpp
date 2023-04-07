// #include <iostream>
#include <map>
#include <string>
#include <cstring>

#include "sys_calls_warp_arounds.hpp"
#include "Server.hpp"
#include "typedefs.hpp"
#include "LocationContext.hpp"

CSV_maps_t CSV_maps;
GlobalContext GlobalContextSingleton;
std::vector<string> g_env;
char** g_env2;

void	BuildEnv(char **env)	{
	for (std::size_t i = 0; env[i]; i++)
		g_env.push_back(env[i]);
}

int main(int ac, char **av, char **env)
{
	try
	{
		Server server;
		BuildEnv(env);
		string config_file_path = ac >= 2 ? av[1] : "internal_server_ressources/default_config_file";
		server.Run(config_file_path);
	}
	catch(const std::exception& e)
	{
		std::cerr << e.what() << '\n';
	}
}   

// int main()
// {
//     HTTP_Response response = HTTP_Response::mk_from_file_and_status_code("200" , "./web_ressources/7468015_800.jpg");
//     // std::cout << response.debug() << std::endl;
//     std::string str;
//     try
//     {
//         parsing::line_of_tokens_t content_type = response.get_header_fields("Content-Type");
//         if (content_type.size() >= 1 && content_type[1].find("text"))
//         {
//             std::cout << "content_type[0] = " << content_type[0] << std::endl;
//             str.append(response.body);
//             str.append(parsing::CLRF);
//         }
//         else
//             str.append("HTTP message body was ommited because it is not text.");
//     }
//     catch(HTTP_Message::NoHeaderFieldFoundException e) 
//     {  
//         std::cout << "Could not found Content-Type header field while debugging response." << std::endl;
//     }
//     std::cout << str << std::endl;
//     // std::cout << response.serialize() << std::endl;
// }

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