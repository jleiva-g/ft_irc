NAME		=	ircserv

H_FILES		=	Server.hpp Client.hpp Channel.hpp Command.hpp Utils.hpp Exceptions.hpp
H_DIR		=	inc
HEADERS		=	$(addprefix $(H_DIR)/, $(H_FILES))

SRC_FILES	=	main.cpp \
				Server.cpp Client.cpp Channel.cpp Command.cpp Utils.cpp Exceptions.cpp
SRC_DIR		=	src
SRC			=	$(addprefix $(SRC_DIR)/, $(SRC_FILES))

OBJ_DIR		=	obj
OBJ			=	$(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRC))

CC			=	c++
CFLAGS		=	-Wall -Wextra -Werror -std=c++98

#Colors
GREEN		=	\033[0;32m
YELLOW		=	\033[0;33m
RED			=	\033[0;31m
PURPLE		=	\033[0;34m
PINK		=	\033[0;35m
CYAN		=	\033[0;36m
DEFAULT		=	\033[0m

all: $(NAME)

$(NAME): $(OBJ)
	@$(CC) $(CFLAGS) -I$(H_DIR) $(OBJ) -o $(NAME)
	@printf "$(PINK)Compilation successful. Created $(NAME)$(DEFAULT)\n"

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp $(HEADERS) | $(OBJ_DIR)
	@$(CC) $(CFLAGS) -I$(H_DIR) -c $< -o $@
	@printf "$(CYAN)Compiled $< into $@$(DEFAULT)\n"

$(OBJ_DIR):
	@mkdir -p $@
	@printf "$(YELLOW)Created directory $(OBJ_DIR)$(DEFAULT)\n"

clean:
	@rm -rf $(OBJ_DIR)
	@printf "$(RED)Cleaned object files$(DEFAULT)\n"

fclean: clean
	@rm -rf $(NAME)
	@printf "$(PURPLE)Cleaned executable$(DEFAULT)\n"

re: fclean all

.PHONY: all clean fclean re