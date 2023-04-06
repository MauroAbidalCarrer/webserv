#ifndef TYPEDEFS_HPP
# define TYPEDEFS_HPP
# include <iostream>
# include <string>

//using namespaces
using std::string;
using std::vector;
using std::runtime_error;
using std::pair;
using std::cout;
using std::cerr;
using std::endl;

//parsing
typedef std::vector<std::string> string_vec_t;
typedef string_vec_t::iterator string_vec_it_t;
typedef std::pair<std::string&, std::string*> directive_fields_dsts_t;

//CSV maps
typedef std::map<std::string, std::string> str_to_str_map_t;
typedef std::map<std::string, std::map<std::string, std::string> > CSV_maps_t;

//ANISI codes
# define RED_AINSI "\e[38;5;9m"
# define BOLD_AINSI "\e[1m"
# define FAINT_AINSI "\e[2m"
# define END_AINSI "\e[0m"
#endif