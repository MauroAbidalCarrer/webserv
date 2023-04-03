NAME	:= webserv
INCS	=	-I srcs/HTTP_Message\
			-I srcs/utils\
			-I srcs

SRCS	:= srcs/main.cpp
OBJS	:= $(SRCS:.cpp=.o)
DEPS	=	$(OBJS:.o=.d)

CXX	:= c++
CXXFLAGS := -std=c++98 -g3 -Wall -Werror -Wextra -MMD -MP 

all: $(NAME)

$(NAME): $(OBJS)
	mkdir -p objs
	$(CXX) $(OBJS) -o $@

%.o: %.cpp 
	$(CXX) $(INCS) $(CXXFLAGS) -o $@ -c $<

clean:
	rm -f $(OBJS) $(DEPS)

fclean: clean
	rm -f $(NAME)

re:	fclean all

.PHONY: all clean fclean re

-include $(DEPS)