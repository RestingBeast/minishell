/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   envp_init.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: psoe <psoe@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/10 19:08:17 by psoe              #+#    #+#             */
/*   Updated: 2026/10/10 19:08:17 by psoe             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "minishell.h"

/*
** Adds one "KEY=value" string from envp to our list.
** Like bash, entries with no '=' or an empty KEY are skipped,
** and when a KEY appears twice the last value wins.
*/
static int	import_entry(t_list **env, char *entry)
{
	char	*equal;
	char	*key;
	int		status;

	equal = ft_strchr(entry, '=');
	if (equal == NULL || equal == entry)
		return (0);
	key = ft_substr(entry, 0, equal - entry);
	if (key == NULL)
		return (-1);
	status = env_set(env, key, equal + 1);
	free(key);
	return (status);
}

/*
** Like bash, a shell started without PATH (env -i ./minishell) still finds
** commands: it gets DEFAULT_PATH. Unlike bash, which keeps it as a shell
** variable, it is exported here, so env and child programs see it too.
** A PATH that is set but empty is kept as it is.
*/
static int	set_default_path(t_list **env)
{
	if (env_find(*env, "PATH") != NULL)
		return (0);
	return (env_set(env, "PATH", DEFAULT_PATH));
}

/*
** Copies the environment that main() received into our own list,
** then fixes PWD, SHLVL, OLDPWD and PATH like bash does at startup.
** Returns 0, or -1 if malloc failed.
** Works when envp is empty (env -i ./minishell).
*/
int	env_init(t_list **env, char **envp)
{
	int	i;

	*env = NULL;
	i = 0;
	while (envp != NULL && envp[i] != NULL)
	{
		if (import_entry(env, envp[i]) == -1)
		{
			ft_lstclear(env, free);
			return (-1);
		}
		i++;
	}
	if (update_pwd(env) == -1 || update_shlvl(env) == -1
		|| update_oldpwd(env) == -1 || set_default_path(env) == -1)
	{
		ft_lstclear(env, free);
		return (-1);
	}
	return (0);
}
