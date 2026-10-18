#include "test.h"
#include <criterion/criterion.h>
#include <criterion/redirect.h>
#include <unistd.h>

/*
** Every expected result below was checked against real bash.
** $? is 42 in all tests, unless a test says otherwise.
*/
#define STATUS 42

/*
** The env list used by every test:
**   USER=psoe  HOME=/home/psoe  A="quoted"  B=it's  C=a b  D=$USER  E=
** and NOVAL, which is set but has no value (like "export NOVAL").
** env_init adds PWD, SHLVL and OLDPWD on its own, so they are removed.
*/
static t_list	*make_env(void)
{
	char	*envp[] = {"USER=psoe", "HOME=/home/psoe", "A=\"quoted\"",
		"B=it's", "C=a b", "D=$USER", "E=", NULL};
	t_list	*env;

	cr_assert_eq(env_init(&env, envp), 0);
	env_unset(&env, "PWD");
	env_unset(&env, "SHLVL");
	env_unset(&env, "OLDPWD");
	cr_assert_eq(env_set(&env, "NOVAL", NULL), 0);
	return (env);
}

/*
** Expands one WORD and checks the result.
*/
static void	check_word(char *word, char *expected)
{
	t_list	*env = make_env();
	t_err	err;
	char	*res = expand_word(word, env, STATUS, &err);

	cr_assert_not_null(res, "expand_word(%s) returned NULL", word);
	cr_assert_str_eq(res, expected, "%s should give [%s], got [%s]",
		word, expected, res);
	free(res);
	ft_lstclear(&env, free);
}

/*
** Runs the real lexer and parser on LINE, like the shell does.
*/
static t_node	*parse_line(char *line, t_list **tokens)
{
	t_data	data;
	t_node	*tree;

	*tokens = lexer(line, &data.err);
	cr_assert_eq(data.err.type, OK, "lexer failed on: %s", line);
	tree = parse_tokens(*tokens, &data);
	cr_assert_eq(data.err.type, OK, "parser failed on: %s", line);
	return (tree);
}

/*
** Checks that the arguments of CMD are exactly EXPECTED.
*/
static void	check_cmd_args(t_cmd *cmd, char **expected, char *line)
{
	int	i;

	i = 0;
	while (expected[i] != NULL && cmd->args[i] != NULL)
	{
		cr_assert_str_eq(cmd->args[i], expected[i],
			"%s: argument %d should be [%s], got [%s]",
			line, i, expected[i], cmd->args[i]);
		i++;
	}
	cr_assert_null(cmd->args[i], "%s: extra argument [%s]",
		line, cmd->args[i]);
	cr_assert_null(expected[i], "%s: missing argument [%s]",
		line, expected[i]);
}

/*
** Parses and expands LINE (one command, no pipe), then checks its arguments.
*/
static void	check_args(char *line, char **expected)
{
	t_list	*env = make_env();
	t_list	*tokens;
	t_node	*tree = parse_line(line, &tokens);
	t_err	err;

	cr_assert_eq(expand_tree(tree, env, STATUS, &err), 0);
	cr_assert_not_null(tree);
	cr_assert_eq(tree->type, COMMAND);
	check_cmd_args((t_cmd *)tree->data, expected, line);
	ft_nodeclear(tree);
	free_tokens(tokens);
	ft_lstclear(&env, free);
}

/*
** The N-th redirection (counting from 0) of CMD.
*/
static t_redir	*redir_at(t_cmd *cmd, int n)
{
	t_list	*lst = cmd->redir;

	while (n-- > 0 && lst != NULL)
		lst = lst->next;
	cr_assert_not_null(lst, "redirection is missing");
	return ((t_redir *)lst->content);
}

/*
** The file name of the N-th redirection (counting from 0) of CMD.
*/
static char	*redir_file(t_cmd *cmd, int n)
{
	return (redir_at(cmd, n)->target.file);
}

/*
** Parses and expands LINE (one command, no pipe), then checks the file
** name of its first redirection and if it is ambiguous.
*/
static void	check_redir(char *line, char *file, int ambiguous)
{
	t_list	*env = make_env();
	t_list	*tokens;
	t_node	*tree = parse_line(line, &tokens);
	t_err	err;
	t_redir	*redir;

	cr_assert_eq(expand_tree(tree, env, STATUS, &err), 0);
	redir = redir_at((t_cmd *)tree->data, 0);
	cr_assert_str_eq(redir->target.file, file,
		"%s: file should be [%s], got [%s]", line, file, redir->target.file);
	cr_assert_eq(redir->ambiguous, ambiguous,
		"%s: ambiguous should be %d", line, ambiguous);
	ft_nodeclear(tree);
	free_tokens(tokens);
	ft_lstclear(&env, free);
}

static void	check_strip(char *str, char *expected)
{
	char	*res = strip_quotes(ft_strdup(str));

	cr_assert_not_null(res);
	cr_assert_str_eq(res, expected, "%s should give [%s], got [%s]",
		str, expected, res);
	free(res);
}

/* ----------------------------------------------------------- variables */

// Command: echo $USER $HOME
Test(ExpandVar, replaces_variable_by_its_value)
{
	check_word("$USER", "psoe");
	check_word("$HOME", "/home/psoe");
}

// Command: echo hi$USER $USER$HOME
Test(ExpandVar, variable_inside_a_word)
{
	check_word("hi$USER", "hipsoe");
	check_word("$USER$HOME", "psoe/home/psoe");
	check_word("a$USER-b", "apsoe-b");
}

// Command: echo $USER.txt $USER/docs
Test(ExpandVar, name_stops_at_first_char_that_is_not_a_name_char)
{
	check_word("$USER.txt", "psoe.txt");
	check_word("$USER/docs", "psoe/docs");
	check_word("$USER=x", "psoe=x");
}

// Command: echo $USER_abc   (_ is part of a name: looks for USER_abc)
Test(ExpandVar, underscore_and_digits_are_part_of_the_name)
{
	check_word("$USER_abc", "");
	check_word("$USER2", "");
	check_word("$_X", "");
}

// Command: echo "[$NOPE]"
Test(ExpandVar, missing_variable_becomes_empty)
{
	check_word("$NOPE", "");
	check_word("[$NOPE]", "[]");
	check_word("hello$NOPE!", "hello!");
}

// Command: export NOVAL ; echo "[$NOVAL]"   and   echo "[$E]" with E=
Test(ExpandVar, variable_without_value_or_empty_value_becomes_empty)
{
	check_word("[$NOVAL]", "[]");
	check_word("[$E]", "[]");
}

// Command: echo $A $B $D   (A="quoted", B=it's, D=$USER)
Test(ExpandVar, quotes_and_dollar_in_a_value_are_kept)
{
	check_word("$A", "\"quoted\"");
	check_word("$B", "it's");
	check_word("$D", "$USER");
	check_word("\"$D\"", "$USER");
}

// Command: echo $C   (C=a b: no word splitting, the subject does not ask)
Test(ExpandVar, value_with_spaces_stays_one_word)
{
	check_word("$C", "a b");
	check_word("\"$C\"", "a b");
}

// expand_word makes a new string and does not change the word it reads.
Test(ExpandVar, original_word_is_not_changed)
{
	char	word[] = "\"$USER\"";
	t_list	*env = make_env();
	t_err	err;
	char	*res = expand_word(word, env, STATUS, &err);

	cr_assert_str_eq(res, "psoe");
	cr_assert_str_eq(word, "\"$USER\"");
	free(res);
	ft_lstclear(&env, free);
}

/* ------------------------------------------------------------------ $? */

// Command: echo $? "$?"
Test(ExpandStatus, becomes_exit_status)
{
	check_word("$?", "42");
	check_word("\"$?\"", "42");
}

// Command: echo '$?'
Test(ExpandStatus, not_expanded_in_single_quotes)
{
	check_word("'$?'", "$?");
}

// Command: echo $?$? $?? x$?y
Test(ExpandStatus, inside_words)
{
	check_word("$?$?", "4242");
	check_word("$??", "42?");
	check_word("x$?y", "x42y");
}

// $? after commands that ended with 0, 127 and 255
Test(ExpandStatus, other_status_values)
{
	t_list	*env = make_env();
	t_err	err;
	char	*res0 = expand_word("$?", env, 0, &err);
	char	*res127 = expand_word("$?", env, 127, &err);
	char	*res255 = expand_word("$?", env, 255, &err);

	cr_assert_str_eq(res0, "0");
	cr_assert_str_eq(res127, "127");
	cr_assert_str_eq(res255, "255");
	free(res0);
	free(res127);
	free(res255);
	ft_lstclear(&env, free);
}

/* --------------------------------------------------------------- quotes */

// Command: echo '$USER' '$USER is in $HOME'
Test(ExpandQuotes, single_quotes_expand_nothing)
{
	check_word("'$USER'", "$USER");
	check_word("'$USER is in $HOME'", "$USER is in $HOME");
	check_word("'$NOPE'", "$NOPE");
}

// Command: echo "$USER" "$USER is in $HOME"
Test(ExpandQuotes, double_quotes_still_expand_dollar)
{
	check_word("\"$USER\"", "psoe");
	check_word("\"$USER is in $HOME\"", "psoe is in /home/psoe");
	check_word("\"$USER$NOPE$HOME\"", "psoe/home/psoe");
}

// Command: echo "'$USER'" '"$USER"'
Test(ExpandQuotes, other_quote_inside_quotes_is_a_normal_char)
{
	check_word("\"'$USER'\"", "'psoe'");
	check_word("'\"$USER\"'", "\"$USER\"");
	check_word("\"it's\"", "it's");
	check_word("'\"hi\"'", "\"hi\"");
}

// Command: echo "$USER"'$USER'  $USER"$USER"'$USER'$USER
Test(ExpandQuotes, mixed_quotes_in_one_word)
{
	check_word("\"$USER\"'$USER'", "psoe$USER");
	check_word("$USER\"$USER\"'$USER'$USER", "psoepsoe$USERpsoe");
	check_word("\"a'b\"'c\"d'", "a'bc\"d");
}

// Command: echo hello "hello" he"ll"o
Test(ExpandQuotes, quotes_are_removed)
{
	check_word("hello", "hello");
	check_word("\"hello\"", "hello");
	check_word("he\"ll\"o", "hello");
}

// Command: echo "" '' '''' """" "''"
Test(ExpandQuotes, empty_quotes)
{
	check_word("\"\"", "");
	check_word("''", "");
	check_word("''''", "");
	check_word("\"\"\"\"", "");
	check_word("\"''\"", "''");
}

/* ----------------------------------------------------- tricky dollars */

// Command: echo $ "$" '$'
Test(ExpandDollar, lone_dollar_is_kept)
{
	check_word("$", "$");
	check_word("\"$\"", "$");
	check_word("'$'", "$");
	check_word("\"$ \"", "$ ");
}

// Command: echo $. $= $/
Test(ExpandDollar, dollar_before_a_char_that_cannot_start_a_name)
{
	check_word("$.", "$.");
	check_word("$=", "$=");
	check_word("$/", "$/");
}

// Command: echo $1 $1abc "$9x"   (a digit is a script argument: we have none)
Test(ExpandDollar, dollar_digit_takes_only_one_digit)
{
	check_word("$1", "");
	check_word("$1abc", "abc");
	check_word("\"$9x\"", "x");
	check_word("$12", "2");
}

// Command: echo $"USER" $'USER' $'' $""
Test(ExpandDollar, dollar_before_quotes_is_dropped)
{
	check_word("$\"USER\"", "USER");
	check_word("$'USER'", "USER");
	check_word("$''", "");
	check_word("$\"\"", "");
}

// Command: echo "$'USER'" "$"USER   (inside "...", $ before a quote is kept)
Test(ExpandDollar, dollar_before_quote_inside_double_quotes_is_kept)
{
	check_word("\"$'USER'\"", "$'USER'");
	check_word("\"$\"USER", "$USER");
}

/* -------------------------------------------------- arguments of a command */

// Command: echo $USER "$HOME"
Test(ExpandArgs, all_arguments_are_expanded)
{
	char	*expected[] = {"echo", "psoe", "/home/psoe", NULL};

	check_args("echo $USER \"$HOME\"", expected);
}

// Command: $NOPE echo hi   (runs echo hi)
Test(ExpandArgs, empty_unquoted_argument_is_removed)
{
	char	*expected1[] = {"echo", "hi", NULL};
	char	*expected2[] = {"echo", "a", "b", NULL};

	check_args("$NOPE echo hi", expected1);
	check_args("echo a $NOPE b", expected2);
}

// Command: echo "$NOPE" $NOPE"" '' x
Test(ExpandArgs, empty_argument_with_quotes_is_kept)
{
	char	*expected[] = {"echo", "", "", "", "x", NULL};

	check_args("echo \"$NOPE\" $NOPE\"\" '' x", expected);
}

// Command: $NOPE $E   (nothing left: an empty command)
Test(ExpandArgs, every_argument_removed_gives_empty_command)
{
	char	*expected[] = {NULL};

	check_args("$NOPE $E", expected);
}

// Command: $USER   (the command name is expanded too)
Test(ExpandArgs, command_name_is_expanded)
{
	char	*expected[] = {"psoe", NULL};

	check_args("$USER", expected);
}

/* ------------------------------------------------------------ the tree */

// expand_tree on an empty line
Test(ExpandTree, empty_tree)
{
	t_list	*env = make_env();
	t_err	err;

	cr_assert_eq(expand_tree(NULL, env, STATUS, &err), 0);
	ft_lstclear(&env, free);
}

// Command: echo $USER | cat "$HOME" | wc '$X'
Test(ExpandTree, every_command_of_a_pipeline_is_expanded)
{
	char	*line = "echo $USER | cat \"$HOME\" | wc '$X'";
	char	*expected1[] = {"echo", "psoe", NULL};
	char	*expected2[] = {"cat", "/home/psoe", NULL};
	char	*expected3[] = {"wc", "$X", NULL};
	t_list	*env = make_env();
	t_list	*tokens;
	t_node	*tree = parse_line(line, &tokens);
	t_err	err;

	cr_assert_eq(expand_tree(tree, env, STATUS, &err), 0);
	cr_assert_eq(tree->type, PIPE);
	cr_assert_eq(tree->left->type, PIPE);
	check_cmd_args((t_cmd *)tree->left->left->data, expected1, line);
	check_cmd_args((t_cmd *)tree->left->right->data, expected2, line);
	check_cmd_args((t_cmd *)tree->right->data, expected3, line);
	ft_nodeclear(tree);
	free_tokens(tokens);
	ft_lstclear(&env, free);
}

// Command: ls | $NOPE   (the right side becomes an empty command)
Test(ExpandTree, empty_command_in_a_pipeline)
{
	char	*line = "ls | $NOPE";
	char	*expected_left[] = {"ls", NULL};
	char	*expected_right[] = {NULL};
	t_list	*env = make_env();
	t_list	*tokens;
	t_node	*tree = parse_line(line, &tokens);
	t_err	err;

	cr_assert_eq(expand_tree(tree, env, STATUS, &err), 0);
	cr_assert_eq(tree->type, PIPE);
	check_cmd_args((t_cmd *)tree->left->data, expected_left, line);
	cr_assert_not_null(tree->right);
	check_cmd_args((t_cmd *)tree->right->data, expected_right, line);
	ft_nodeclear(tree);
	free_tokens(tokens);
	ft_lstclear(&env, free);
}

// Command: echo hi > $HOME/out >> "$USER.log" < '$IN'
Test(ExpandTree, redirection_files_are_expanded)
{
	char	*line = "echo hi > $HOME/out >> \"$USER.log\" < '$IN'";
	t_list	*env = make_env();
	t_list	*tokens;
	t_node	*tree = parse_line(line, &tokens);
	t_cmd	*cmd;
	t_err	err;

	cr_assert_eq(expand_tree(tree, env, STATUS, &err), 0);
	cmd = (t_cmd *)tree->data;
	cr_assert_str_eq(redir_file(cmd, 0), "/home/psoe/out");
	cr_assert_str_eq(redir_file(cmd, 1), "psoe.log");
	cr_assert_str_eq(redir_file(cmd, 2), "$IN");
	ft_nodeclear(tree);
	free_tokens(tokens);
	ft_lstclear(&env, free);
}

// Command: > "$NOPE" echo x   (the file name "" is kept, bash says
// "No such file or directory" when it opens it)
Test(ExpandTree, empty_redirection_file_is_kept)
{
	char	*line = "> \"$NOPE\" echo x";
	char	*expected[] = {"echo", "x", NULL};
	t_list	*env = make_env();
	t_list	*tokens;
	t_node	*tree = parse_line(line, &tokens);
	t_cmd	*cmd;
	t_err	err;

	cr_assert_eq(expand_tree(tree, env, STATUS, &err), 0);
	cmd = (t_cmd *)tree->data;
	check_cmd_args(cmd, expected, line);
	cr_assert_str_eq(redir_file(cmd, 0), "");
	cr_assert_eq(redir_at(cmd, 0)->ambiguous, 0);
	ft_nodeclear(tree);
	free_tokens(tokens);
	ft_lstclear(&env, free);
}

// Command: echo hi > $NOPE   (bash: "$NOPE: ambiguous redirect")
// The file keeps the word as it was typed, for the error message.
Test(ExpandTree, empty_unquoted_redirection_file_is_ambiguous)
{
	check_redir("echo hi > $NOPE", "$NOPE", 1);
	check_redir("echo hi >> $NOPE", "$NOPE", 1);
	check_redir("cat < $NOPE", "$NOPE", 1);
	check_redir("echo hi > $E", "$E", 1);
	check_redir("echo hi > $NOPE$E", "$NOPE$E", 1);
}

// Command: echo hi > "$NOPE"   (bash: ": No such file or directory")
Test(ExpandTree, quoted_or_not_empty_redirection_file_is_not_ambiguous)
{
	check_redir("echo hi > \"$NOPE\"", "", 0);
	check_redir("echo hi > $NOPE\"\"", "", 0);
	check_redir("echo hi > ''", "", 0);
	check_redir("echo hi > $USER", "psoe", 0);
}

// Command: cat << $USER   then the lines "line" and "$USER"
// bash never expands the delimiter: the heredoc stops at the text $USER.
Test(ExpandTree, heredoc_delimiter_is_not_expanded,
	.init = cr_redirect_stdout, .timeout = 5)
{
	t_list	*env = make_env();
	t_list	*tokens;
	t_node	*tree;
	t_redir	*redir;
	char	buf[64];
	ssize_t	len;
	FILE	*input;
	t_err	err;

	cr_redirect_stdin();
	input = cr_get_redirected_stdin();
	fprintf(input, "line\n$USER\n");
	fclose(input);
	tree = parse_line("cat << $USER", &tokens);
	cr_assert_eq(expand_tree(tree, env, STATUS, &err), 0);
	redir = (t_redir *)((t_cmd *)tree->data)->redir->content;
	cr_assert_eq(redir->type, HEREDOC);
	len = read(redir->target.fd, buf, sizeof(buf) - 1);
	cr_assert_geq(len, 0);
	buf[len] = '\0';
	cr_assert_str_eq(buf, "line\n");
	ft_nodeclear(tree);
	free_tokens(tokens);
	ft_lstclear(&env, free);
}

/* -------------------------------------------------------- strip_quotes */

// Used for the heredoc delimiter: quotes are removed, nothing is expanded.
Test(StripQuotes, removes_only_the_outer_quotes)
{
	check_strip("\"it's\"", "it's");
	check_strip("'\"hi\"'", "\"hi\"");
	check_strip("he\"ll\"o", "hello");
	check_strip("'a'\"b\"c", "abc");
	check_strip("\"''\"", "''");
}

Test(StripQuotes, does_not_expand)
{
	check_strip("\"$USER\"", "$USER");
	check_strip("$USER", "$USER");
}

Test(StripQuotes, empty_results)
{
	check_strip("", "");
	check_strip("''", "");
	check_strip("\"\"", "");
}

Test(StripQuotes, null_gives_null)
{
	cr_assert_null(strip_quotes(NULL));
}

/* -------------------------------------------------------- malloc fails */

/*
** These tests make each malloc fail in turn: the 1st, then the 2nd...
** until a run where no malloc failed. Each failed run must return
** NULL (or -1), set err to MALLOC_ERROR and leak nothing
** (AddressSanitizer checks the leaks). The run that works must
** leave err at OK.
*/

Test(ExpandMalloc, expand_word_handles_every_failure)
{
	char	*word = "a\"$USER\"'$HOME'$?$NOPE$1x";
	t_list	*env = make_env();
	t_err	err;
	char	*res;
	int		n;

	n = 1;
	while (1)
	{
		err.type = OK;
		fail_malloc_at(n);
		res = expand_word(word, env, STATUS, &err);
		if (stop_failing() < n)
			break ;
		cr_assert_null(res, "malloc #%d failed but the result is [%s]",
			n, res);
		cr_assert_eq(err.type, MALLOC_ERROR,
			"malloc #%d failed but err is not MALLOC_ERROR", n);
		n++;
	}
	cr_assert_gt(n, 1);
	cr_assert_eq(err.type, OK);
	cr_assert_str_eq(res, "apsoe$HOME42x");
	free(res);
	ft_lstclear(&env, free);
}

Test(ExpandMalloc, expand_tree_handles_every_failure)
{
	char	*line = "echo $USER\"$HOME\" $NOPE '' | cat > $HOME/f < $NOPE";
	char	*expected_left[] = {"echo", "psoe/home/psoe", "", NULL};
	char	*expected_right[] = {"cat", NULL};
	t_list	*env = make_env();
	t_list	*tokens;
	t_node	*tree;
	t_err	err;
	int		ret;
	int		n;

	n = 1;
	while (1)
	{
		tree = parse_line(line, &tokens);
		err.type = OK;
		fail_malloc_at(n);
		ret = expand_tree(tree, env, STATUS, &err);
		if (stop_failing() < n)
			break ;
		cr_assert_eq(ret, -1, "malloc #%d failed but expand_tree worked", n);
		cr_assert_eq(err.type, MALLOC_ERROR,
			"malloc #%d failed but err is not MALLOC_ERROR", n);
		ft_nodeclear(tree);
		free_tokens(tokens);
		n++;
	}
	cr_assert_gt(n, 1);
	cr_assert_eq(ret, 0);
	cr_assert_eq(err.type, OK);
	check_cmd_args((t_cmd *)tree->left->data, expected_left, line);
	check_cmd_args((t_cmd *)tree->right->data, expected_right, line);
	cr_assert_str_eq(redir_file((t_cmd *)tree->right->data, 0),
		"/home/psoe/f");
	cr_assert_str_eq(redir_file((t_cmd *)tree->right->data, 1), "$NOPE");
	cr_assert_eq(redir_at((t_cmd *)tree->right->data, 1)->ambiguous, 1);
	ft_nodeclear(tree);
	free_tokens(tokens);
	ft_lstclear(&env, free);
}

// strip_quotes always frees the string it gets, even when malloc fails.
Test(ExpandMalloc, strip_quotes_frees_input_when_malloc_fails)
{
	char	*str = ft_strdup("\"hello\"");

	fail_malloc_at(1);
	cr_assert_null(strip_quotes(str));
	stop_failing();
}
