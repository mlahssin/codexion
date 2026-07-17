NAME    = codexion

CC      = cc


CFLAGS  = -Wall -Wextra -Werror -pthread  -I include
# CFLAGS  = -Wall -Wextra -Werror -I include
SRCS    =  src1/main.c \
           src1/parsing.c \
 		   src1/params_init.c\
		   src1/params_init_utils.c\
		   src1/heap.c \
		   src1/heap_utils.c \
		   src1/time_utils.c \
		   src1/finish_check.c \
		   src1/dongle_rules.c \
		   src1/dongle_access.c \
		   src1/logging.c \
		   src1/coder_routine.c \
		   src1/cleanup.c \
		   src1/setup.c \

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
