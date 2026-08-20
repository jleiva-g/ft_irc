NAME		=	ircserv

H_FILES		=	Server.hpp Client.hpp Channel.hpp Command.hpp Utils.hpp Exceptions.hpp
H_DIR		=	inc
HEADERS		=	$(addprefix $(H_DIR)/, $(H_FILES))

SRC_FILES	=	main.cpp \
				Server.cpp Client.cpp Channel.cpp Command.cpp Utils.cpp Exceptions.cpp
SRC_DIR		=	src
SRC			=	$(addprefix $(SRC_DIR)/, $(SRC_FILES))

OBJ_DIR		=	$(SRC_DIR)/obj
OBJ			=	$(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRC))

CC			=	c++
CFLAGS		=	-Wall -Wextra -Werror -std=c++98

all: $(NAME)

$(NAME): $(OBJ)
	$(CC) $(CFLAGS) -I$(H_DIR) $(OBJ) -o $(NAME)
	@echo "Compilation successful. Created $(NAME)"

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp $(HEADERS) | $(OBJ_DIR)
	$(CC) $(CFLAGS) -I$(H_DIR) -c $< -o $@

$(OBJ_DIR):
	@mkdir -p $@

clean:
	@rm -rf $(OBJ_DIR)

fclean: clean
	@rm -rf $(NAME)

re: fclean all

.PHONY: all clean fclean re