/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   exec_path.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kkhant-z <kkhant-z@student.42singapore.sg  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/17 22:33:31 by kkhant-z          #+#    #+#             */
/*   Updated: 2026/10/17 22:33:32 by kkhant-z         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "minishell.h"

static int	is_exec_file(char *file)
{
	struct stat	statbuf;

	if (stat(file, &statbuf) == -1 || S_ISDIR(statbuf.st_mode))
		return (0);
	if (access(file, X_OK) != 0)
		return (-1);
	return (1);
}

static char	*join_path(char *dir, char *cmd)
{
	char	*tmp;
	char	*res;

	tmp = ft_strjoin(dir, "/");
	if (!tmp)
		return (NULL);
	res = ft_strjoin(tmp, cmd);
	free(tmp);
	return (res);
}

/*
** Looks for cmd in each directory of PATH. Frees dirs before returning.
*/
static char	*search_path(char *cmd, char **dirs, t_err *err)
{
	char	*res;
	char	*saved;
	int		i;
	int		flag;

	i = -1;
	saved = NULL;
	while (dirs[++i] != NULL)
	{
		res = join_path(dirs[i], cmd);
		if (!res)
			return (ft_argclear(dirs), set_malloc_err(err), free(saved), NULL);
		flag = is_exec_file(res);
		if (flag == 1)
			return (ft_argclear(dirs), free(saved), res);
		if (flag == -1 && !saved)
			saved = res;
		else
			free(res);
	}
	ft_argclear(dirs);
	if (saved)
		return (saved);
	return (set_cmd_not_found_err(err, cmd), NULL);
}

/*
** A command with a '/' is used as it is, no PATH search. Like bash, so is
** any command when PATH is unset: it is looked up in the cwd.
** An empty PATH is different: it is searched like PATH=".", so a miss
** is still "command not found".
** Returns a copy, so the caller can always free the result.
*/
static char	*check_slash_cmd(char *cmd, t_err *err)
{
	struct stat	statbuf;
	char		*res;

	if (stat(cmd, &statbuf) == -1)
	{
		if (errno == ENOENT)
		{
			set_cmd_not_found_err(err, cmd);
			err->err_no = ENOENT;
			return (NULL);
		}
		return (set_cmd_err(err, cmd), NULL);
	}
	if (S_ISDIR(statbuf.st_mode))
		return (set_dir_err(err, cmd), NULL);
	res = ft_strdup(cmd);
	if (!res)
		return (set_malloc_err(err), NULL);
	return (res);
}

char	*find_command(char *cmd, t_data *data)
{
	char	*path;
	char	**dirs;

	if (cmd[0] == '\0')
		return (set_cmd_not_found_err(&(data->err), cmd), NULL);
	if (ft_strchr(cmd, '/') != NULL)
		return (check_slash_cmd(cmd, &(data->err)));
	path = env_get(data->env, "PATH");
	if (!path)
		return (check_slash_cmd(cmd, &(data->err)));
	if (!*path)
		path = ".";
	dirs = ft_split(path, ':');
	if (!dirs)
		return (set_malloc_err(&(data->err)), NULL);
	return (search_path(cmd, dirs, &(data->err)));
}
