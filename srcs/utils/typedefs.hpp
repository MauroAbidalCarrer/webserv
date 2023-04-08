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

//networking
typedef struct addrinfo addrinfo_t;
typedef struct sockaddr sockaddr_t;

//CSV maps
typedef std::map<std::string, std::string> str_to_str_map_t;
typedef std::map<std::string, std::map<std::string, std::string> > CSV_maps_t;

//ANISI codes
//colors
# define RED_AINSI "\e[38;5;9m"
# define BLUE_AINSI "\e[38;5;14m"
# define YELLOW_AINSI "\e[38;5;11m"
//intensity
# define BOLD_AINSI "\e[1m"
# define FAINT_AINSI "\e[2m"
//words
# define RED_ERROR "\e[38;5;9mError\e[0m: "
# define YELLOW_WARNING "\e[38;5;11mWarning\e[0m: "

# define END_AINSI "\e[0m"

#include <sstream>
#define SSTR( x ) static_cast< std::ostringstream & >( ( std::ostringstream() << std::dec << x ) ).str()

#endif