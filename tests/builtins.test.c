#include "minishell.h"
#include <criterion/criterion.h>
#include <criterion/redirect.h>
#include <limits.h>
#include <sys/stat.h>
#include <unistd.h>

/*
** Makes an env list holding exactly ENVP: env_init adds PWD, SHLVL,
** OLDPWD and DEFAULT_PATH on its own, so they are removed again.
*/
static t_list	*make_env(char **envp)
{
	t_list	*env;

	cr_assert_eq(env_init(&env, envp), 0);
	env_unset(&env, "PWD");
	env_unset(&env, "SHLVL");
	env_unset(&env, "OLDPWD");
	if (ft_strncmp(env_get(env, "PATH"), DEFAULT_PATH, sizeof(DEFAULT_PATH)) == 0)
		env_unset(&env, "PATH");
	return (env);
}

/*
** Makes an empty directory in /tmp and returns its full path (malloc'd).
*/
static char	*make_temp_dir(void)
{
	char	path[] = "/tmp/minishell_builtin_XXXXXX";

	cr_assert_not_null(mkdtemp(path));
	return (ft_strdup(path));
}

/*
** cd tests change the directory of the test process: go back after each.
*/
static char	g_start_dir[PATH_MAX];

static void	save_dir_and_redirect(void)
{
	cr_assert_not_null(getcwd(g_start_dir, sizeof(g_start_dir)));
	cr_redirect_stdout();
	cr_redirect_stderr();
}

static void	restore_dir(void)
{
	chdir(g_start_dir);
}

/*
** exit prints "exit" only when stdin is a terminal. Redirecting stdin
** makes it a pipe, so the tests give the same output everywhere.
*/
static void	no_terminal(void)
{
	cr_redirect_stdin();
	cr_redirect_stderr();
}

/* ---------------------------------------------------------------- echo */

// Command: echo hello world
Test(Echo, prints_arguments_with_spaces, .init = cr_redirect_stdout)
{
	char	*argv[] = {"echo", "hello", "world", NULL};

	cr_assert_eq(builtin_echo(argv), 0);
	cr_assert_stdout_eq_str("hello world\n");
}

// Command: echo
Test(Echo, no_arguments_prints_newline, .init = cr_redirect_stdout)
{
	char	*argv[] = {"echo", NULL};

	cr_assert_eq(builtin_echo(argv), 0);
	cr_assert_stdout_eq_str("\n");
}

// Command: echo -n hello
Test(Echo, n_option_removes_newline, .init = cr_redirect_stdout)
{
	char	*argv[] = {"echo", "-n", "hello", NULL};

	cr_assert_eq(builtin_echo(argv), 0);
	cr_assert_stdout_eq_str("hello");
}

// Command: echo -n
Test(Echo, n_option_alone_prints_nothing, .init = cr_redirect_stdout)
{
	char	*argv[] = {"echo", "-n", NULL};

	cr_assert_eq(builtin_echo(argv), 0);
	cr_assert_stdout_eq_str("");
}

// Command: echo -n -nnn -n hi
Test(Echo, many_n_options, .init = cr_redirect_stdout)
{
	char	*argv[] = {"echo", "-n", "-nnn", "-n", "hi", NULL};

	cr_assert_eq(builtin_echo(argv), 0);
	cr_assert_stdout_eq_str("hi");
}

// Command: echo -nx a ; echo - a ; echo --n a ; echo -n-n a
Test(Echo, things_that_look_like_options, .init = cr_redirect_stdout)
{
	char	*argv1[] = {"echo", "-nx", "a", NULL};
	char	*argv2[] = {"echo", "-", "a", NULL};
	char	*argv3[] = {"echo", "--n", "a", NULL};
	char	*argv4[] = {"echo", "-n-n", "a", NULL};

	builtin_echo(argv1);
	builtin_echo(argv2);
	builtin_echo(argv3);
	builtin_echo(argv4);
	cr_assert_stdout_eq_str("-nx a\n- a\n--n a\n-n-n a\n");
}

// Command: echo a -n   (-n after a word is just a word)
Test(Echo, option_after_word_is_printed, .init = cr_redirect_stdout)
{
	char	*argv[] = {"echo", "-n", "a", "-n", NULL};

	builtin_echo(argv);
	cr_assert_stdout_eq_str("a -n");
}

// Command: echo "" a ; echo a "" b
Test(Echo, empty_arguments_still_get_a_space, .init = cr_redirect_stdout)
{
	char	*argv1[] = {"echo", "", "a", NULL};
	char	*argv2[] = {"echo", "a", "", "b", NULL};

	builtin_echo(argv1);
	builtin_echo(argv2);
	cr_assert_stdout_eq_str(" a\na  b\n");
}

/* ----------------------------------------------------------------- pwd */

// Command: pwd
Test(PwdBuiltin, prints_current_directory, .init = cr_redirect_stdout)
{
	char	*cwd = getcwd(NULL, 0);
	char	*expected = ft_strjoin(cwd, "\n");
	t_list	*env = make_env(NULL);

	cr_assert_eq(builtin_pwd(env), 0);
	cr_assert_stdout_eq_str(expected);
	free(cwd);
	free(expected);
	ft_lstclear(&env, free);
}

// Command: mkdir d; cd d; rmdir ../d; pwd
Test(PwdBuiltin, deleted_directory_uses_pwd_variable,
	.init = save_dir_and_redirect, .fini = restore_dir)
{
	char	*dir = make_temp_dir();
	char	*expected = ft_strjoin(dir, "\n");
	t_list	*env = make_env(NULL);

	env_set(&env, "PWD", dir);
	cr_assert_eq(chdir(dir), 0);
	cr_assert_eq(rmdir(dir), 0);
	cr_assert_eq(builtin_pwd(env), 0);
	cr_assert_stdout_eq_str(expected);
	free(dir);
	free(expected);
	ft_lstclear(&env, free);
}

Test(PwdBuiltin, deleted_directory_and_no_pwd_variable,
	.init = save_dir_and_redirect, .fini = restore_dir)
{
	char	*dir = make_temp_dir();
	t_list	*env = make_env(NULL);

	cr_assert_eq(chdir(dir), 0);
	cr_assert_eq(rmdir(dir), 0);
	cr_assert_eq(builtin_pwd(env), 1);
	cr_assert_stdout_eq_str("");
	cr_assert_stderr_eq_str("minishell: pwd: error retrieving current "
		"directory: getcwd: No such file or directory\n");
	free(dir);
}

/* ------------------------------------------------------------------ cd */

// Command: cd /tmp/minishell_builtin_XXXXXX
Test(Cd, absolute_path_updates_pwd_and_oldpwd,
	.init = save_dir_and_redirect, .fini = restore_dir)
{
	char	*dir = make_temp_dir();
	char	*argv[] = {"cd", dir, NULL};
	t_list	*env = make_env(NULL);
	char	*cwd;

	env_set(&env, "PWD", "/old/place");
	cr_assert_eq(builtin_cd(argv, &env), 0);
	cwd = getcwd(NULL, 0);
	cr_assert_str_eq(cwd, dir);
	cr_assert_str_eq(env_get(env, "PWD"), dir);
	cr_assert_str_eq(env_get(env, "OLDPWD"), "/old/place");
	cr_assert_stderr_eq_str("");
	rmdir(dir);
	free(dir);
	free(cwd);
	ft_lstclear(&env, free);
}

// Command: cd sub ; cd ..
Test(Cd, relative_paths, .init = save_dir_and_redirect, .fini = restore_dir)
{
	char	*dir = make_temp_dir();
	char	*sub = ft_strjoin(dir, "/sub");
	char	*argv1[] = {"cd", "sub", NULL};
	char	*argv2[] = {"cd", "..", NULL};
	t_list	*env = make_env(NULL);

	cr_assert_eq(mkdir(sub, 0755), 0);
	cr_assert_eq(chdir(dir), 0);
	env_set(&env, "PWD", dir);
	cr_assert_eq(builtin_cd(argv1, &env), 0);
	cr_assert_str_eq(env_get(env, "PWD"), sub);
	cr_assert_str_eq(env_get(env, "OLDPWD"), dir);
	cr_assert_eq(builtin_cd(argv2, &env), 0);
	cr_assert_str_eq(env_get(env, "PWD"), dir);
	cr_assert_str_eq(env_get(env, "OLDPWD"), sub);
	rmdir(sub);
	rmdir(dir);
	free(sub);
	free(dir);
	ft_lstclear(&env, free);
}

// Command: cd   (goes to $HOME)
Test(Cd, no_argument_goes_home,
	.init = save_dir_and_redirect, .fini = restore_dir)
{
	char	*dir = make_temp_dir();
	char	*argv[] = {"cd", NULL};
	t_list	*env = make_env(NULL);

	env_set(&env, "HOME", dir);
	cr_assert_eq(builtin_cd(argv, &env), 0);
	cr_assert_str_eq(env_get(env, "PWD"), dir);
	rmdir(dir);
	free(dir);
	ft_lstclear(&env, free);
}

// Command: unset HOME; cd
Test(Cd, no_argument_and_no_home,
	.init = save_dir_and_redirect, .fini = restore_dir)
{
	char	*argv[] = {"cd", NULL};
	t_list	*env = make_env(NULL);
	char	*before = getcwd(NULL, 0);
	char	*after;

	cr_assert_eq(builtin_cd(argv, &env), 1);
	after = getcwd(NULL, 0);
	cr_assert_str_eq(after, before);
	cr_assert_stderr_eq_str("minishell: cd: HOME not set\n");
	free(before);
	free(after);
	ft_lstclear(&env, free);
}

// Command: cd /tmp /   (a usage error: status 2, like bash --posix)
Test(Cd, too_many_arguments,
	.init = save_dir_and_redirect, .fini = restore_dir)
{
	char	*argv[] = {"cd", "/tmp", "/", NULL};
	t_list	*env = make_env(NULL);

	cr_assert_eq(builtin_cd(argv, &env), 2);
	cr_assert_null(env_find(env, "PWD"));
	cr_assert_stderr_eq_str("minishell: cd: too many arguments\n");
	ft_lstclear(&env, free);
}

// Command: cd /nonexistent_dir
Test(Cd, missing_directory,
	.init = save_dir_and_redirect, .fini = restore_dir)
{
	char	*argv[] = {"cd", "/nonexistent_dir", NULL};
	t_list	*env = make_env(NULL);

	env_set(&env, "PWD", "/same");
	env_set(&env, "OLDPWD", "/before");
	cr_assert_eq(builtin_cd(argv, &env), 1);
	cr_assert_str_eq(env_get(env, "PWD"), "/same");
	cr_assert_str_eq(env_get(env, "OLDPWD"), "/before");
	cr_assert_stderr_eq_str(
		"minishell: cd: /nonexistent_dir: No such file or directory\n");
	ft_lstclear(&env, free);
}

// Command: cd /etc/passwd
Test(Cd, not_a_directory, .init = save_dir_and_redirect, .fini = restore_dir)
{
	char	*argv[] = {"cd", "/etc/passwd", NULL};
	t_list	*env = make_env(NULL);

	cr_assert_eq(builtin_cd(argv, &env), 1);
	cr_assert_stderr_eq_str("minishell: cd: /etc/passwd: Not a directory\n");
	ft_lstclear(&env, free);
}

// Command: cd ""   (an error in bash --posix: nothing changes)
Test(Cd, empty_argument_is_an_error,
	.init = save_dir_and_redirect, .fini = restore_dir)
{
	char	*argv[] = {"cd", "", NULL};
	t_list	*env = make_env(NULL);
	char	*cwd = getcwd(NULL, 0);

	env_set(&env, "PWD", cwd);
	cr_assert_eq(builtin_cd(argv, &env), 1);
	cr_assert_str_eq(env_get(env, "PWD"), cwd);
	cr_assert_null(env_find(env, "OLDPWD"));
	cr_assert_stderr_eq_str("minishell: cd: null directory\n");
	free(cwd);
	ft_lstclear(&env, free);
}

// Command: unset PWD; cd /   (bash then removes OLDPWD, and sets PWD again)
Test(Cd, without_pwd_removes_oldpwd,
	.init = save_dir_and_redirect, .fini = restore_dir)
{
	char	*argv[] = {"cd", "/", NULL};
	t_list	*env = make_env(NULL);

	env_set(&env, "OLDPWD", "/before");
	cr_assert_eq(builtin_cd(argv, &env), 0);
	cr_assert_null(env_find(env, "OLDPWD"));
	cr_assert_str_eq(env_get(env, "PWD"), "/");
	ft_lstclear(&env, free);
}

// Command: mkdir d; cd d; rmdir ../d; cd .
Test(Cd, deleted_directory_warns_but_works,
	.init = save_dir_and_redirect, .fini = restore_dir)
{
	char	*dir = make_temp_dir();
	char	*argv[] = {"cd", ".", NULL};
	t_list	*env = make_env(NULL);

	env_set(&env, "PWD", dir);
	cr_assert_eq(chdir(dir), 0);
	cr_assert_eq(rmdir(dir), 0);
	cr_assert_eq(builtin_cd(argv, &env), 0);
	cr_assert_str_eq(env_get(env, "PWD"), dir);
	cr_assert_str_eq(env_get(env, "OLDPWD"), dir);
	cr_assert_stderr_eq_str("minishell: cd: error retrieving current "
		"directory: getcwd: No such file or directory\n");
	free(dir);
	ft_lstclear(&env, free);
}

/* ----------------------------------------------------------------- env */

// Command: env
Test(Env, prints_variables_with_value, .init = cr_redirect_stdout)
{
	char	*envp[] = {"A=1", "B=", NULL};
	char	*argv[] = {"env", NULL};
	t_list	*env = make_env(envp);

	env_set(&env, "NOVALUE", NULL);
	cr_assert_eq(builtin_env(argv, &env), 0);
	cr_assert_stdout_eq_str("A=1\nB=\n_=/usr/bin/env\n");
	ft_lstclear(&env, free);
}

// Command: env foo
Test(Env, argument_is_an_error, .init = cr_redirect_stderr)
{
	char	*argv[] = {"env", "foo", NULL};
	t_list	*env = make_env(NULL);

	cr_assert_eq(builtin_env(argv, &env), 127);
	cr_assert_stderr_eq_str(
		"minishell: env: foo: No such file or directory\n");
	ft_lstclear(&env, free);
}

/* -------------------------------------------------------------- export */

// Command: export A=1 B=x=y C=
Test(Export, sets_variables)
{
	char	*argv[] = {"export", "A=1", "B=x=y", "C=", NULL};
	t_list	*env = make_env(NULL);

	cr_assert_eq(builtin_export(argv, &env), 0);
	cr_assert_str_eq(env_get(env, "A"), "1");
	cr_assert_str_eq(env_get(env, "B"), "x=y");
	cr_assert_str_eq(env_get(env, "C"), "");
	ft_lstclear(&env, free);
}

// Command: export A=2
Test(Export, replaces_existing_value)
{
	char	*envp[] = {"A=1", NULL};
	char	*argv[] = {"export", "A=2", NULL};
	t_list	*env = make_env(envp);

	cr_assert_eq(builtin_export(argv, &env), 0);
	cr_assert_eq(ft_lstsize(env), 1);
	cr_assert_str_eq(env_get(env, "A"), "2");
	ft_lstclear(&env, free);
}

// Command: export A NEW   (A already has a value, NEW does not exist)
Test(Export, name_without_value)
{
	char	*envp[] = {"A=1", NULL};
	char	*argv[] = {"export", "A", "NEW", NULL};
	t_list	*env = make_env(envp);

	cr_assert_eq(builtin_export(argv, &env), 0);
	cr_assert_str_eq(env_get(env, "A"), "1");
	cr_assert_not_null(env_find(env, "NEW"));
	cr_assert_null(env_get(env, "NEW"));
	ft_lstclear(&env, free);
}

// Command: export _A1=x a_b=y
Test(Export, underscores_and_digits_are_allowed)
{
	char	*argv[] = {"export", "_A1=x", "a_b=y", "_=z", NULL};
	t_list	*env = make_env(NULL);

	cr_assert_eq(builtin_export(argv, &env), 0);
	cr_assert_str_eq(env_get(env, "_A1"), "x");
	cr_assert_str_eq(env_get(env, "a_b"), "y");
	cr_assert_str_eq(env_get(env, "_"), "z");
	ft_lstclear(&env, free);
}

// Command: export 1A=2 OK=3 =x A-B ""
Test(Export, bad_names_are_errors_but_others_still_work,
	.init = cr_redirect_stderr)
{
	char	*argv[] = {"export", "1A=2", "OK=3", "=x", "A-B", "", NULL};
	t_list	*env = make_env(NULL);

	cr_assert_eq(builtin_export(argv, &env), 1);
	cr_assert_eq(ft_lstsize(env), 1);
	cr_assert_str_eq(env_get(env, "OK"), "3");
	cr_assert_stderr_eq_str(
		"minishell: export: `1A=2': not a valid identifier\n"
		"minishell: export: `=x': not a valid identifier\n"
		"minishell: export: `A-B': not a valid identifier\n"
		"minishell: export: `': not a valid identifier\n");
	ft_lstclear(&env, free);
}

// Command: export   (sorted, quoted, _ hidden, no-value names shown)
Test(Export, no_arguments_prints_sorted_list, .init = cr_redirect_stdout)
{
	char	*envp[] = {"B=a\"b$c\\d`e", "A1=x", "_=/usr/bin/x", "A=1",
		"C=", NULL};
	char	*argv[] = {"export", NULL};
	t_list	*env = make_env(envp);

	env_set(&env, "_Z", NULL);
	env_set(&env, "D", NULL);
	cr_assert_eq(builtin_export(argv, &env), 0);
	cr_assert_stdout_eq_str(
		"declare -x A=\"1\"\n"
		"declare -x A1=\"x\"\n"
		"declare -x B=\"a\\\"b\\$c\\\\d\\`e\"\n"
		"declare -x C=\"\"\n"
		"declare -x D\n"
		"declare -x _Z\n");
	ft_lstclear(&env, free);
}

// Sorting is by name: "A" < "A1" < "AB" < "A_" < "a" (like strcmp)
Test(Export, sorts_by_name_only, .init = cr_redirect_stdout)
{
	char	*envp[] = {"a=1", "A_=1", "AB=1", "A1=1", "A=1", NULL};
	char	*argv[] = {"export", NULL};
	t_list	*env = make_env(envp);

	cr_assert_eq(builtin_export(argv, &env), 0);
	cr_assert_str_eq(env->content, "a=1", "the list itself must not move");
	cr_assert_stdout_eq_str(
		"declare -x A=\"1\"\n"
		"declare -x A1=\"1\"\n"
		"declare -x AB=\"1\"\n"
		"declare -x A_=\"1\"\n"
		"declare -x a=\"1\"\n");
	ft_lstclear(&env, free);
}

Test(Export, empty_env_prints_nothing, .init = cr_redirect_stdout)
{
	char	*argv[] = {"export", NULL};
	t_list	*env = make_env(NULL);

	cr_assert_eq(builtin_export(argv, &env), 0);
	cr_assert_stdout_eq_str("");
}

Test(Export, sort_env_keeps_the_same_strings)
{
	char	*envp[] = {"B=2", "A=1", NULL};
	t_list	*env = make_env(envp);
	char	**array = sort_env(env);

	cr_assert_eq(array[0], env->next->content);
	cr_assert_eq(array[1], env->content);
	cr_assert_null(array[2]);
	free(array);
	ft_lstclear(&env, free);
}

/* --------------------------------------------------------------- unset */

// Command: unset A C
Test(Unset, removes_variables)
{
	char	*envp[] = {"A=1", "B=2", "C=3", NULL};
	char	*argv[] = {"unset", "A", "C", NULL};
	t_list	*env = make_env(envp);

	cr_assert_eq(builtin_unset(argv, &env), 0);
	cr_assert_eq(ft_lstsize(env), 1);
	cr_assert_str_eq(env->content, "B=2");
	ft_lstclear(&env, free);
}

// Command: unset ; unset MISSING
Test(Unset, nothing_to_remove)
{
	char	*envp[] = {"A=1", NULL};
	char	*argv1[] = {"unset", NULL};
	char	*argv2[] = {"unset", "MISSING", NULL};
	t_list	*env = make_env(envp);

	cr_assert_eq(builtin_unset(argv1, &env), 0);
	cr_assert_eq(builtin_unset(argv2, &env), 0);
	cr_assert_eq(ft_lstsize(env), 1);
	ft_lstclear(&env, free);
}

// Command: unset A=1 1A ""   (bash 5.1 ignores bad names silently)
Test(Unset, bad_names_are_ignored, .init = cr_redirect_stderr)
{
	char	*envp[] = {"A=1", NULL};
	char	*argv[] = {"unset", "A=1", "1A", "", NULL};
	t_list	*env = make_env(envp);

	cr_assert_eq(builtin_unset(argv, &env), 0);
	cr_assert_str_eq(env_get(env, "A"), "1");
	cr_assert_stderr_eq_str("");
	ft_lstclear(&env, free);
}

// Command: export NOVALUE; unset NOVALUE
Test(Unset, removes_name_without_value)
{
	char	*argv[] = {"unset", "NOVALUE", NULL};
	t_list	*env = make_env(NULL);

	env_set(&env, "NOVALUE", NULL);
	cr_assert_eq(builtin_unset(argv, &env), 0);
	cr_assert_null(env_find(env, "NOVALUE"));
}

/* ---------------------------------------------------------------- exit */

static void	check_exit(char *arg, int expected)
{
	char	*argv[] = {"exit", arg, NULL};
	int		should_exit = 0;
	int		code = builtin_exit(argv, 0, &should_exit);

	cr_assert_eq(code, expected, "exit \"%s\" should give %d, got %d",
		arg, expected, code);
	cr_assert_eq(should_exit, 1, "exit \"%s\" should quit the shell", arg);
}

// Command: false; exit   (uses $?)
Test(Exit, no_argument_uses_last_status, .init = no_terminal)
{
	char	*argv[] = {"exit", NULL};
	int		should_exit = 0;

	cr_assert_eq(builtin_exit(argv, 42, &should_exit), 42);
	cr_assert_eq(should_exit, 1);
	cr_assert_stderr_eq_str("");
}

Test(Exit, numbers, .init = no_terminal)
{
	check_exit("0", 0);
	check_exit("42", 42);
	check_exit("255", 255);
	check_exit("256", 0);
	check_exit("300", 44);
	check_exit("-1", 255);
	check_exit("-256", 0);
	check_exit("+5", 5);
	check_exit(" 5 ", 5);
	check_exit("\t7\n", 7);
	check_exit("0000000042", 42);
	cr_assert_stderr_eq_str("");
}

Test(Exit, long_long_limits, .init = no_terminal)
{
	check_exit("9223372036854775807", 255);
	check_exit("-9223372036854775808", 0);
	check_exit("9223372036854775808", 2);
	check_exit("-9223372036854775809", 2);
	check_exit("99999999999999999999999", 2);
}

Test(Exit, not_numbers, .init = no_terminal)
{
	check_exit("abc", 2);
	check_exit("", 2);
	check_exit("1x", 2);
	check_exit("5 5", 2);
	check_exit("--5", 2);
	check_exit("+", 2);
	check_exit("-", 2);
	check_exit(" ", 2);
}

// Command: exit abc
Test(Exit, not_a_number_message, .init = no_terminal)
{
	check_exit("abc", 2);
	cr_assert_stderr_eq_str(
		"minishell: exit: abc: numeric argument required\n");
}

// Command: exit 1 2   (error with status 2, and the shell keeps running)
Test(Exit, too_many_arguments, .init = no_terminal)
{
	char	*argv[] = {"exit", "1", "2", NULL};
	int		should_exit = 1;

	cr_assert_eq(builtin_exit(argv, 0, &should_exit), 2);
	cr_assert_eq(should_exit, 0);
	cr_assert_stderr_eq_str("minishell: exit: too many arguments\n");
}

// Command: exit abc 2   (the first argument is checked first: exits with 2)
Test(Exit, bad_number_wins_over_too_many, .init = no_terminal)
{
	char	*argv[] = {"exit", "abc", "2", NULL};
	int		should_exit = 0;

	cr_assert_eq(builtin_exit(argv, 0, &should_exit), 2);
	cr_assert_eq(should_exit, 1);
	cr_assert_stderr_eq_str(
		"minishell: exit: abc: numeric argument required\n");
}

/* ------------------------------------------------------------ dispatch */

Test(Builtin, knows_every_builtin)
{
	cr_assert(is_builtin("echo"));
	cr_assert(is_builtin("cd"));
	cr_assert(is_builtin("pwd"));
	cr_assert(is_builtin("export"));
	cr_assert(is_builtin("unset"));
	cr_assert(is_builtin("env"));
	cr_assert(is_builtin("exit"));
}

Test(Builtin, other_names_are_not_builtins)
{
	cr_assert_not(is_builtin("ls"));
	cr_assert_not(is_builtin("ech"));
	cr_assert_not(is_builtin("echoo"));
	cr_assert_not(is_builtin("ECHO"));
	cr_assert_not(is_builtin(""));
	cr_assert_not(is_builtin(NULL));
}

Test(Builtin, runs_the_right_builtin, .init = cr_redirect_stdout)
{
	char	*argv[] = {"echo", "hi", NULL};
	t_list	*env = make_env(NULL);
	int		should_exit = 1;

	cr_assert_eq(run_builtin(argv, &env, 0, &should_exit), 0);
	cr_assert_eq(should_exit, 0);
	cr_assert_stdout_eq_str("hi\n");
}

Test(Builtin, run_changes_the_env)
{
	char	*argv1[] = {"export", "A=1", NULL};
	char	*argv2[] = {"unset", "A", NULL};
	t_list	*env = make_env(NULL);
	int		should_exit;

	cr_assert_eq(run_builtin(argv1, &env, 0, &should_exit), 0);
	cr_assert_str_eq(env_get(env, "A"), "1");
	cr_assert_eq(run_builtin(argv2, &env, 0, &should_exit), 0);
	cr_assert_null(env);
}

Test(Builtin, run_exit_asks_to_quit, .init = no_terminal)
{
	char	*argv[] = {"exit", "7", NULL};
	t_list	*env = make_env(NULL);
	int		should_exit = 0;

	cr_assert_eq(run_builtin(argv, &env, 0, &should_exit), 7);
	cr_assert_eq(should_exit, 1);
}
