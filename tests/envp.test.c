#include "test.h"
#include <criterion/criterion.h>
#include <criterion/redirect.h>
#include <unistd.h>

static t_list	*make_env(char **envp)
{
	t_list	*env;

	cr_assert_eq(env_init(&env, envp), 0);
	return (env);
}

/*
** Same as make_env, but without the variables env_init adds on its own,
** so the list holds exactly what the test gave it.
*/
static t_list	*plain_env(char **envp)
{
	t_list	*env = make_env(envp);

	env_unset(&env, "PWD");
	env_unset(&env, "SHLVL");
	env_unset(&env, "OLDPWD");
	return (env);
}

static void	check_shlvl(char *entry, char *expected)
{
	char	*envp[] = {entry, NULL};
	t_list	*env = make_env(envp);

	cr_assert_str_eq(env_get(env, "SHLVL"), expected,
		"\"%s\" should give SHLVL=%s, got SHLVL=%s",
		entry, expected, env_get(env, "SHLVL"));
	ft_lstclear(&env, free);
}

/* ---------------------------------------------------------------- init */

Test(EnvInit, copies_every_variable)
{
	char	*envp[] = {"A=1", "B=hello world", "C=x=y", NULL};
	t_list	*env = make_env(envp);

	cr_assert_eq(ft_lstsize(env), 6);
	cr_assert_str_eq(env_get(env, "A"), "1");
	cr_assert_str_eq(env_get(env, "B"), "hello world");
	cr_assert_str_eq(env_get(env, "C"), "x=y");
	cr_assert_str_eq(env_get(env, "SHLVL"), "1");
	ft_lstclear(&env, free);
}

Test(EnvInit, makes_its_own_copy)
{
	char	entry[] = "A=1";
	char	*envp[] = {entry, NULL};
	t_list	*env = make_env(envp);

	entry[2] = '9';
	cr_assert_str_eq(env_get(env, "A"), "1");
	ft_lstclear(&env, free);
}

static void	check_startup_only(t_list *env)
{
	char	*cwd = getcwd(NULL, 0);

	cr_assert_eq(ft_lstsize(env), 3);
	cr_assert_str_eq(env_get(env, "PWD"), cwd);
	cr_assert_str_eq(env_get(env, "SHLVL"), "1");
	cr_assert_not_null(env_find(env, "OLDPWD"));
	cr_assert_null(env_get(env, "OLDPWD"));
	free(cwd);
}

// Command: env -i ./minishell
Test(EnvInit, empty_environment)
{
	char	*envp[] = {NULL};
	t_list	*env = make_env(envp);

	check_startup_only(env);
	ft_lstclear(&env, free);
}

Test(EnvInit, null_envp)
{
	t_list	*env = make_env(NULL);

	check_startup_only(env);
	ft_lstclear(&env, free);
}

Test(EnvInit, duplicate_key_keeps_last_value)
{
	char	*envp[] = {"A=1", "B=2", "A=3", NULL};
	t_list	*env = plain_env(envp);

	cr_assert_eq(ft_lstsize(env), 2);
	cr_assert_str_eq(env_get(env, "A"), "3");
	env_unset(&env, "A");
	cr_assert_null(env_find(env, "A"));
	ft_lstclear(&env, free);
}

Test(EnvInit, skips_entries_without_equal_or_name)
{
	char	*envp[] = {"NOEQUALS", "", "=value", "A-B=1", NULL};
	t_list	*env = plain_env(envp);

	cr_assert_eq(ft_lstsize(env), 1);
	cr_assert_str_eq(env->content, "A-B=1");
	ft_lstclear(&env, free);
}

Test(Shlvl, duplicate_shlvl_uses_last_value)
{
	char	*envp[] = {"SHLVL=2", "SHLVL=7", NULL};
	t_list	*env = make_env(envp);

	cr_assert_str_eq(env_get(env, "SHLVL"), "8");
	env_unset(&env, "SHLVL");
	cr_assert_null(env_find(env, "SHLVL"));
	ft_lstclear(&env, free);
}

/* --------------------------------------------------------------- SHLVL */

Test(Shlvl, adds_one)
{
	check_shlvl("SHLVL=0", "1");
	check_shlvl("SHLVL=3", "4");
	check_shlvl("SHLVL=998", "999");
}

Test(Shlvl, invalid_value_becomes_one)
{
	check_shlvl("SHLVL=", "1");
	check_shlvl("SHLVL=abc", "1");
	check_shlvl("SHLVL=1x", "1");
	check_shlvl("SHLVL=-", "1");
	check_shlvl("SHLVL=   ", "1");
	check_shlvl("SHLVL=99999999999", "1");
}

// "SHLVL" has no '=', so it is skipped on import: same as no SHLVL at all.
Test(Shlvl, missing_becomes_one)
{
	check_shlvl("SHLVL", "1");
	check_shlvl("OTHER=5", "1");
}

Test(Shlvl, leading_zeros_and_minus_zero)
{
	check_shlvl("SHLVL=0000000000000000000005", "6");
	check_shlvl("SHLVL=-0", "1");
}

Test(Shlvl, numbers_too_big_for_long_become_one)
{
	check_shlvl("SHLVL=99999999999999999999999", "1");
	check_shlvl("SHLVL=-99999999999999999999", "1");
}

Test(Shlvl, near_int_limits, .init = cr_redirect_stderr)
{
	check_shlvl("SHLVL=-2147483647", "0");
	check_shlvl("SHLVL=2147483646", "1");
	cr_assert_stderr_eq_str("minishell: warning: shell level (2147483647)"
		" too high, resetting to 1\n");
}

Test(Shlvl, spaces_and_plus_sign_are_allowed)
{
	check_shlvl("SHLVL= 7 ", "8");
	check_shlvl("SHLVL=\t7\n", "8");
	check_shlvl("SHLVL=+2", "3");
}

Test(Shlvl, negative_becomes_zero)
{
	check_shlvl("SHLVL=-1", "0");
	check_shlvl("SHLVL=-5", "0");
}

Test(Shlvl, too_high_resets_to_one, .init = cr_redirect_stderr)
{
	check_shlvl("SHLVL=999", "1");
	cr_assert_stderr_eq_str(
		"minishell: warning: shell level (1000) too high, resetting to 1\n");
}

Test(Shlvl, keeps_its_place_in_the_list)
{
	char	*envp[] = {"A=1", "SHLVL=2", "B=2", NULL};
	t_list	*env = make_env(envp);

	cr_assert_str_eq(env->next->content, "SHLVL=3");
	ft_lstclear(&env, free);
}

/* ---------------------------------------------------------- PWD/OLDPWD */

Test(Pwd, wrong_or_relative_pwd_is_replaced)
{
	char	*envp[] = {"PWD=/nonexistent", NULL};
	char	*envp2[] = {"PWD=.", NULL};
	char	*cwd = getcwd(NULL, 0);
	t_list	*env = make_env(envp);
	t_list	*env2 = make_env(envp2);

	cr_assert_str_eq(env_get(env, "PWD"), cwd);
	cr_assert_str_eq(env_get(env2, "PWD"), cwd);
	free(cwd);
	ft_lstclear(&env, free);
	ft_lstclear(&env2, free);
}

// PWD is a real directory, just not the one we are in.
Test(Pwd, other_existing_dir_is_replaced)
{
	char	*envp[] = {"PWD=/", NULL};
	char	*cwd = getcwd(NULL, 0);
	t_list	*env;

	cr_assert_str_neq(cwd, "/", "run the tests from a directory other than /");
	env = make_env(envp);
	cr_assert_str_eq(env_get(env, "PWD"), cwd);
	free(cwd);
	ft_lstclear(&env, free);
}

// Command: cd into a symlink, then start ./minishell (bash keeps the link)
Test(Pwd, symlink_to_current_dir_is_kept)
{
	char	*cwd = getcwd(NULL, 0);
	char	link[64];
	char	*entry;
	t_list	*env;

	snprintf(link, sizeof(link), "/tmp/minishell_test_link_%d", getpid());
	unlink(link);
	cr_assert_eq(symlink(cwd, link), 0);
	entry = ft_strjoin("PWD=", link);
	char	*envp[] = {entry, NULL};
	env = make_env(envp);
	unlink(link);
	cr_assert_str_eq(env_get(env, "PWD"), link);
	free(cwd);
	free(entry);
	ft_lstclear(&env, free);
}

Test(Pwd, correct_pwd_is_kept_in_place)
{
	char	*cwd = getcwd(NULL, 0);
	char	*entry = ft_strjoin("PWD=", cwd);
	char	*envp[] = {entry, "A=1", NULL};
	t_list	*env = make_env(envp);

	cr_assert_str_eq(env->content, entry);
	free(cwd);
	free(entry);
	ft_lstclear(&env, free);
}

Test(Oldpwd, missing_is_added_without_value)
{
	t_list	*env = make_env(NULL);

	cr_assert_not_null(env_find(env, "OLDPWD"));
	cr_assert_null(env_get(env, "OLDPWD"));
	ft_lstclear(&env, free);
}

Test(Oldpwd, directory_is_kept)
{
	char	*envp[] = {"OLDPWD=/", NULL};
	t_list	*env = make_env(envp);

	cr_assert_str_eq(env_get(env, "OLDPWD"), "/");
	ft_lstclear(&env, free);
}

Test(Oldpwd, not_a_directory_is_removed)
{
	char	*envp[] = {"OLDPWD=/nonexistent", NULL};
	t_list	*env = make_env(envp);

	cr_assert_null(env_find(env, "OLDPWD"));
	ft_lstclear(&env, free);
}

Test(Oldpwd, regular_file_is_removed)
{
	char	path[] = "/tmp/minishell_test_XXXXXX";
	int		fd = mkstemp(path);
	char	*entry;
	t_list	*env;

	cr_assert_neq(fd, -1);
	close(fd);
	entry = ft_strjoin("OLDPWD=", path);
	char	*envp[] = {entry, NULL};
	env = make_env(envp);
	unlink(path);
	cr_assert_null(env_find(env, "OLDPWD"));
	free(entry);
	ft_lstclear(&env, free);
}

/* ----------------------------------------------------------------- get */

Test(EnvGet, missing_key_returns_null)
{
	char	*envp[] = {"A=1", NULL};
	t_list	*env = make_env(envp);

	cr_assert_null(env_get(env, "B"));
	ft_lstclear(&env, free);
}

Test(EnvGet, does_not_match_part_of_a_key)
{
	char	*envp[] = {"PATHX=1", NULL};
	t_list	*env = make_env(envp);

	cr_assert_null(env_get(env, "PATH"));
	cr_assert_null(env_get(env, "PAT"));
	cr_assert_null(env_get(env, "PATHXY"));
	cr_assert_str_eq(env_get(env, "PATHX"), "1");
	ft_lstclear(&env, free);
}

Test(EnvGet, empty_value_is_not_null)
{
	char	*envp[] = {"A=", NULL};
	t_list	*env = make_env(envp);

	cr_assert_str_eq(env_get(env, "A"), "");
	ft_lstclear(&env, free);
}

Test(EnvGet, key_without_value_returns_null)
{
	t_list	*env = make_env(NULL);

	cr_assert_eq(env_set(&env, "A", NULL), 0);
	cr_assert_not_null(env_find(env, "A"));
	cr_assert_null(env_get(env, "A"));
	ft_lstclear(&env, free);
}

/* ----------------------------------------------------------------- set */

// Command: export NEW=value
Test(EnvSet, adds_new_variable_at_the_end)
{
	char	*envp[] = {"A=1", NULL};
	t_list	*env = plain_env(envp);

	cr_assert_eq(env_set(&env, "NEW", "value"), 0);
	cr_assert_eq(ft_lstsize(env), 2);
	cr_assert_str_eq(ft_lstlast(env)->content, "NEW=value");
	ft_lstclear(&env, free);
}

// Command: export A=2
Test(EnvSet, replaces_existing_value)
{
	char	*envp[] = {"A=1", NULL};
	t_list	*env = plain_env(envp);

	cr_assert_eq(env_set(&env, "A", "2"), 0);
	cr_assert_eq(ft_lstsize(env), 1);
	cr_assert_str_eq(env->content, "A=2");
	ft_lstclear(&env, free);
}

Test(EnvSet, empty_value)
{
	t_list	*env = make_env(NULL);

	cr_assert_eq(env_set(&env, "A", ""), 0);
	cr_assert_str_eq(env_get(env, "A"), "");
	ft_lstclear(&env, free);
}

// Command: export A   (A already has a value)
Test(EnvSet, null_value_keeps_existing_value)
{
	char	*envp[] = {"A=1", NULL};
	t_list	*env = make_env(envp);

	cr_assert_eq(env_set(&env, "A", NULL), 0);
	cr_assert_str_eq(env_get(env, "A"), "1");
	ft_lstclear(&env, free);
}

// Command: export A   (A is not set yet)
Test(EnvSet, null_value_adds_key_without_value)
{
	t_list	*env = plain_env(NULL);

	cr_assert_eq(env_set(&env, "A", NULL), 0);
	cr_assert_str_eq(ft_lstlast(env)->content, "A");
	cr_assert_null(env_get(env, "A"));
	ft_lstclear(&env, free);
}

// Command: export PATH=2   (only PATHX and PA exist)
Test(EnvSet, similar_keys_are_not_replaced)
{
	char	*envp[] = {"PATHX=1", "PA=0", NULL};
	t_list	*env = plain_env(envp);

	cr_assert_eq(env_set(&env, "PATH", "2"), 0);
	cr_assert_eq(ft_lstsize(env), 3);
	cr_assert_str_eq(env_get(env, "PATHX"), "1");
	cr_assert_str_eq(env_get(env, "PA"), "0");
	cr_assert_str_eq(env_get(env, "PATH"), "2");
	ft_lstclear(&env, free);
}

Test(EnvSet, value_can_come_from_the_list_itself)
{
	char	*envp[] = {"A=hello", NULL};
	t_list	*env = make_env(envp);

	cr_assert_eq(env_set(&env, "A", env_get(env, "A")), 0);
	cr_assert_eq(env_set(&env, "B", env_get(env, "A")), 0);
	cr_assert_str_eq(env_get(env, "A"), "hello");
	cr_assert_str_eq(env_get(env, "B"), "hello");
	ft_lstclear(&env, free);
}

/* --------------------------------------------------------------- unset */

Test(EnvUnset, removes_first_middle_and_last)
{
	char	*envp[] = {"A=1", "B=2", "C=3", "D=4", NULL};
	t_list	*env = plain_env(envp);

	env_unset(&env, "B");
	cr_assert_null(env_find(env, "B"));
	env_unset(&env, "A");
	cr_assert_str_eq(env->content, "C=3");
	env_unset(&env, "D");
	cr_assert_eq(ft_lstsize(env), 1);
	cr_assert_str_eq(env->content, "C=3");
	ft_lstclear(&env, free);
}

Test(EnvUnset, missing_key_does_nothing)
{
	char	*envp[] = {"A=1", NULL};
	t_list	*env = plain_env(envp);

	env_unset(&env, "B");
	env_unset(&env, "");
	cr_assert_eq(ft_lstsize(env), 1);
	ft_lstclear(&env, free);
}

// Command: unset PATH   (PA and PATHX must stay)
Test(EnvUnset, similar_keys_are_kept)
{
	char	*envp[] = {"PA=1", "PATH=2", "PATHX=3", NULL};
	t_list	*env = plain_env(envp);

	env_unset(&env, "PATH");
	cr_assert_eq(ft_lstsize(env), 2);
	cr_assert_str_eq(env->content, "PA=1");
	cr_assert_str_eq(env->next->content, "PATHX=3");
	ft_lstclear(&env, free);
}

Test(EnvUnset, removing_everything_leaves_empty_list)
{
	char	*envp[] = {"A=1", NULL};
	t_list	*env = plain_env(envp);

	env_unset(&env, "A");
	cr_assert_null(env);
	env_unset(&env, "A");
	cr_assert_null(env);
}

Test(EnvUnset, removes_key_without_value)
{
	t_list	*env = make_env(NULL);

	cr_assert_eq(env_set(&env, "A", NULL), 0);
	env_unset(&env, "A");
	cr_assert_null(env_find(env, "A"));
	ft_lstclear(&env, free);
}

/* ------------------------------------------------------------ to_array */

Test(EnvToArray, skips_keys_without_value_and_ends_with_null)
{
	char	*envp[] = {"A=1", NULL};
	t_list	*env = plain_env(envp);

	env_set(&env, "B", NULL);
	env_set(&env, "C", "");
	char	**array = env_to_array(env);

	cr_assert_not_null(array);
	cr_assert_str_eq(array[0], "A=1");
	cr_assert_str_eq(array[1], "C=");
	cr_assert_null(array[2]);
	free_array(array);
	ft_lstclear(&env, free);
}

Test(EnvToArray, strings_are_copies)
{
	char	*envp[] = {"A=1", NULL};
	t_list	*env = make_env(envp);
	char	**array = env_to_array(env);

	cr_assert_neq(array[0], env->content);
	free_array(array);
	ft_lstclear(&env, free);
}

Test(EnvToArray, empty_list)
{
	char	**array = env_to_array(NULL);

	cr_assert_not_null(array);
	cr_assert_null(array[0]);
	free_array(array);
}

/* --------------------------------------------------------------- print */

// Command: env
Test(EnvPrint, prints_only_variables_with_value, .init = cr_redirect_stdout)
{
	char	*envp[] = {"A=1", NULL};
	t_list	*env = plain_env(envp);

	env_set(&env, "B", NULL);
	env_set(&env, "C", "");

	cr_assert_eq(env_print(&env), 0);
	ft_lstclear(&env, free);
	cr_assert_stdout_eq_str("A=1\nC=\n_=/usr/bin/env\n");
}

// Command: ./minishell started from bash, which passes _=./minishell
Test(EnvPrint, replaces_inherited_underscore, .init = cr_redirect_stdout)
{
	char	*envp[] = {"_=./minishell", "A=1", NULL};
	t_list	*env = plain_env(envp);

	cr_assert_eq(env_print(&env), 0);
	cr_assert_eq(ft_lstsize(env), 2);
	ft_lstclear(&env, free);
	cr_assert_stdout_eq_str("_=/usr/bin/env\nA=1\n");
}

/* -------------------------------------------------------- malloc fails */

/*
** Each test makes malloc #1, #2, #3... fail in turn, until the function
** gets through without hitting the failing malloc. Every failure must
** return the error value and leave things as they were. Run the tests
** under valgrind with --no-fork to check that nothing leaks either.
*/

Test(MallocFail, env_init_cleans_up_at_every_step)
{
	char	*envp[] = {"A=1", "SHLVL=4", "PWD=/nonexistent", "A=2", NULL};
	t_list	*env;
	int		n = 0;
	int		status;
	int		calls;

	do
	{
		n++;
		fail_malloc_at(n);
		status = env_init(&env, envp);
		calls = stop_failing();
		if (calls >= n)
		{
			cr_assert_eq(status, -1, "malloc #%d failed, env_init gave %d",
				n, status);
			cr_assert_null(env);
		}
	} while (calls >= n);
	cr_assert_gt(n, 1, "malloc is not wrapped: link with -Wl,--wrap=malloc");
	cr_assert_eq(status, 0);
	ft_lstclear(&env, free);
}

static void	check_set_fails_cleanly(char *key, char *value)
{
	char	*envp[] = {"A=1", "B=2", NULL};
	t_list	*env = plain_env(envp);
	int		n = 0;
	int		status;
	int		calls;

	do
	{
		n++;
		fail_malloc_at(n);
		status = env_set(&env, key, value);
		calls = stop_failing();
		if (calls >= n)
		{
			cr_assert_eq(status, -1, "malloc #%d failed, env_set gave %d",
				n, status);
			cr_assert_eq(ft_lstsize(env), 2);
			cr_assert_str_eq(env->content, "A=1");
			cr_assert_str_eq(env->next->content, "B=2");
		}
	} while (calls >= n);
	cr_assert_gt(n, 1, "malloc is not wrapped: link with -Wl,--wrap=malloc");
	cr_assert_eq(status, 0);
	ft_lstclear(&env, free);
}

Test(MallocFail, env_set_leaves_list_unchanged)
{
	check_set_fails_cleanly("A", "9");
	check_set_fails_cleanly("NEW", "value");
	check_set_fails_cleanly("NEW", NULL);
}

Test(MallocFail, env_to_array_returns_null)
{
	char	*envp[] = {"A=1", "B=2", "C=3", NULL};
	t_list	*env = plain_env(envp);
	char	**array;
	int		n = 0;
	int		calls;

	do
	{
		n++;
		fail_malloc_at(n);
		array = env_to_array(env);
		calls = stop_failing();
		if (calls >= n)
			cr_assert_null(array, "malloc #%d failed but got an array", n);
	} while (calls >= n);
	cr_assert_gt(n, 1, "malloc is not wrapped: link with -Wl,--wrap=malloc");
	cr_assert_str_eq(array[2], "C=3");
	cr_assert_eq(ft_lstsize(env), 3);
	free_array(array);
	ft_lstclear(&env, free);
}

Test(MallocFail, env_print_prints_nothing, .init = cr_redirect_stdout)
{
	char	*envp[] = {"A=1", NULL};
	t_list	*env = plain_env(envp);
	int		status;

	fail_malloc_at(1);
	status = env_print(&env);
	stop_failing();
	cr_assert_eq(status, -1);
	cr_assert_eq(ft_lstsize(env), 1);
	ft_lstclear(&env, free);
	fflush(stdout);
	cr_assert_stdout_eq_str("");
}
