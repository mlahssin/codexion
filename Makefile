NAME    = codexion

CC      = cc


CFLAGS  = -Wall -Wextra -Werror -pthread  -I include

SRCS    =  src1/main.c \
           src1/parsing.c \
 		   src1/params_init.c\
		   src1/heap.c \

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
.SECONDARY:
