/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_cd.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: psoe <psoe@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/12 10:36:10 by psoe              #+#    #+#             */
/*   Updated: 2026/10/12 10:36:10 by psoe             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "minishell.h"
#include <errno.h>
#include <string.h>

/*
** Finds the directory cd should go to:
**   cd        -> $HOME
**   cd -      -> $OLDPWD (builtin_cd() then prints the new directory)
**   cd ""     -> error "null directory", like bash --posix
**   an empty $HOME or $OLDPWD -> "." (stay here, like bash)
**   cd PATH   -> PATH
** Prints an error and returns NULL when there is nowhere to go.
*/
static char	*get_target(char **argv, t_list *env)
{
	char	*target;

	target = argv[1];
	if (target != NULL && target[0] == '\0')
		return (print_error("cd", NULL, "null directory"), NULL);
	if (target == NULL)
	{
		target = env_get(env, "HOME");
		if (target == NULL)
			return (print_error("cd", NULL, "HOME not set"), NULL);
	}
	else if (ft_strncmp(target, "-", 2) == 0)
	{
		target = env_get(env, "OLDPWD");
		if (target == NULL)
			return (print_error("cd", NULL, "OLDPWD not set"), NULL);
	}
	if (target[0] == '\0')
		return (".");
	return (target);
}

/*
** OLDPWD gets the value PWD had before cd.
** Like bash, OLDPWD is removed when PWD was not set.
*/
static int	update_oldpwd_var(t_list **env)
{
	if (env_get(*env, "PWD") == NULL)
	{
		env_unset(env, "OLDPWD");
		return (0);
	}
	return (env_set(env, "OLDPWD", env_get(*env, "PWD")));
}

/*
** PWD gets the directory we are in now.
** If getcwd() fails (the directory was deleted), PWD is left as it was:
** cd still worked, so bash only prints a warning.
*/
static int	update_pwd_var(t_list **env)
{
	char	*cwd;
	int		status;

	cwd = getcwd(NULL, 0);
	if (cwd == NULL)
	{
		print_error("cd", "error retrieving current directory: getcwd",
			strerror(errno));
		return (0);
	}
	status = env_set(env, "PWD", cwd);
	free(cwd);
	return (status);
}

/*
** The cd builtin: changes directory, then updates OLDPWD and PWD.
** Like bash --posix, too many arguments is a usage error: status 2.
** Like bash, "cd -" prints the new directory. It is read from PWD after
** the update: target pointed into the old OLDPWD, which is freed by then.
*/
int	builtin_cd(char **argv, t_list **env)
{
	char	*target;

	if (argv[1] != NULL && argv[2] != NULL)
		return (print_error("cd", NULL, "too many arguments"), 2);
	target = get_target(argv, *env);
	if (target == NULL)
		return (1);
	if (chdir(target) == -1)
	{
		print_error("cd", target, strerror(errno));
		return (1);
	}
	if (update_oldpwd_var(env) == -1 || update_pwd_var(env) == -1)
	{
		print_error("cd", NULL, "out of memory");
		return (1);
	}
	if (argv[1] != NULL && ft_strncmp(argv[1], "-", 2) == 0
		&& env_get(*env, "PWD") != NULL)
		ft_putendl_fd(env_get(*env, "PWD"), STDOUT_FILENO);
	return (0);
}
