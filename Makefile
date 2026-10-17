CC = cc
CFLAGS = -Wall -Wextra -Werror
MAKE = make
NAME = minishell
LIBFT_DIR = libft
LIBFT = $(LIBFT_DIR)/libft.a
READLINE = -lreadline
INCLUDE = -Iinclude -I$(LIBFT_DIR)
SRC = src/minishell.c \
      src/lexer.c \
	  src/lexer_err.c \
	  src/lexer_word.c \
	  src/lexer_utils.c \
	  src/parser.c \
	  src/parser_utils.c \
	  src/heredoc.c \
	  src/heredoc_utils.c \
	  src/node.c \
	  src/node_utils.c \
	  src/utils.c \
	  src/envp.c \
	  src/envp_init.c \
	  src/envp_utils.c \
	  src/shlvl.c \
	  src/pwd.c \
	  src/builtins/builtin.c \
	  src/builtins/builtin_utils.c \
	  src/builtins/builtin_echo.c \
	  src/builtins/builtin_cd.c \
	  src/builtins/builtin_pwd.c \
	  src/builtins/builtin_env.c \
	  src/builtins/builtin_export.c \
	  src/builtins/export_sort.c \
	  src/builtins/export_print.c \
	  src/builtins/builtin_unset.c \
	  src/builtins/builtin_exit.c \
	  src/signals.c \
	  src/signals_exec.c \
	  src/errors.c \
	  src/syntax_err.c \
	  src/syscall_err.c \
	  src/cmd_err.c \
	  src/quotes.c \
	  src/expand.c \
	  src/expand_word.c \
	  src/expand_dollar.c \
	  src/exec.c \
	  src/exec_path.c \
	  src/exec_ext.c \
	  src/exec_builtin.c \
	  src/exec_redir.c \
	  src/exec_utils.c
OBJ = $(SRC:.c=.o)

all: $(LIBFT) $(NAME)

$(LIBFT):
	$(MAKE) -C $(LIBFT_DIR)

$(NAME): $(OBJ) $(LIBFT)
	$(CC) $(CFLAGS) $(OBJ) $(LIBFT) $(READLINE) -o $(NAME)

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDE) -o $@ -c $<

clean:
	rm -f $(OBJ)
	$(MAKE) -C $(LIBFT_DIR) clean

fclean: clean
	rm -f $(NAME)
	$(MAKE) -C $(LIBFT_DIR) fclean

re: fclean all

.PHONY: all clean fclean re
