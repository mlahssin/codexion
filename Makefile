NAME    = codexion

CC      = cc


# CFLAGS  = -Wall -Wextra -Werror -pthread  -I include
CFLAGS  =  -pthread  -g -I include
# - I include means“Add the include/ directory to the compiler's header search path.”
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


# NAME    = codexion

# CC      = cc

# CFLAGS  = -g -I include
# LDFLAGS = -pthread

# SRCS    = src1/main.c \
#           src1/parsing.c \
#           src1/params_init.c

# OBJS    = $(SRCS:.c=.o)

# all: $(NAME)

# $(NAME): $(OBJS)
# 	$(CC) $(OBJS) $(LDFLAGS) -o $(NAME)

# %.o: %.c
# 	$(CC) $(CFLAGS) -c $< -o $@

# clean:
# 	rm -f $(OBJS)

# fclean: clean
# 	rm -f $(NAME)

# re: fclean all

# .PHONY: all clean fclean re