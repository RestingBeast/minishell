/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   exec_ext.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kkhant-z <kkhant-z@student.42singapore.sg  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/17 22:33:23 by kkhant-z          #+#    #+#             */
/*   Updated: 2026/10/17 22:33:24 by kkhant-z         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "minishell.h"

/*
** Only reached in the child when execve() failed.
** Like bash: 127 if the file is gone, 126 for any other reason.
*/
static void	child_fail(char *full_cmd, char **env_arr, t_data *data)
{
	int	err_no;

	err_no = errno;
	ft_putstr_fd("minishell: ", STDERR_FILENO);
	print_cmd_err_msg(CMD_ERROR, err_no, full_cmd);
	free(full_cmd);
	ft_argclear(env_arr);
	clean_exit(data);
	if (err_no == ENOENT)
		exit(127);
	exit(126);
}

static int	dupe_fds(int in_fd, int out_fd)
{
	if (in_fd >= 0 && dup2(in_fd, STDIN_FILENO) == -1)
		return (0);
	if (out_fd >= 0 && dup2(out_fd, STDOUT_FILENO) == -1)
		return (0);
	close_fds(in_fd, out_fd);
	return (1);
}

static void	try_execve(char *full_cmd, t_exec *exec,
		char **env_arr, t_data *data)
{
	set_child_sig();
	close_fds(exec->close_fd, -1);
	if (!dupe_fds(exec->in_fd, exec->out_fd))
		return (close_fds(exec->in_fd, exec->out_fd));
	ft_heredoc_clear(data->tree);
	execve(full_cmd, exec->args, env_arr);
}

void	exec_external(t_exec *exec, t_data *data)
{
	pid_t	pid;
	char	*full_cmd;
	char	**env_arr;

	full_cmd = find_command(exec->args[0], data);
	if (!full_cmd)
		return (close_fds(exec->in_fd, exec->out_fd));
	env_arr = env_to_array(data->env);
	if (!env_arr)
		return (free(full_cmd), close_fds(exec->in_fd, exec->out_fd),
			set_malloc_err(&(data->err)));
	pid = fork();
	if (pid == 0)
	{
		try_execve(full_cmd, exec, env_arr, data);
		child_fail(full_cmd, env_arr, data);
	}
	if (pid == -1)
		set_syscall_err(&(data->err));
	else
		set_last_pid(data, pid);
	close_fds(exec->in_fd, exec->out_fd);
	free(full_cmd);
	ft_argclear(env_arr);
}
