NAME    = codexion

CC      = cc


CFLAGS  = -Wall -Wextra -Werror -I include
# - I include means“Add the include/ directory to the compiler's header search path.”
SRCS    = src/main.c \
          src/parsing.c \
		  src/time_utils.c \
		  src/coders.c \

OBJS    = $(SRCS:.c=.o)

all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(NAME)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
