/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   exec_builtin.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kkhant-z <kkhant-z@student.42singapore.sg  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/17 22:33:15 by kkhant-z          #+#    #+#             */
/*   Updated: 2026/10/17 22:33:17 by kkhant-z         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "minishell.h"

static int	is_one_cmd(t_data *data)
{
	if (data->tree->left == NULL && data->tree->right == NULL)
		return (1);
	return (0);
}

static void	fork_builtin(t_exec *exec, t_data *data)
{
	pid_t	pid;

	pid = fork();
	if (pid == 0)
	{
		close_fds(exec->close_fd, -1);
		set_child_sig();
		if (exec->out_fd >= 0 && dup2(exec->out_fd, STDOUT_FILENO) == -1)
		{
			ft_putstr_fd("minishell: ", STDERR_FILENO);
			ft_putstr_fd(strerror(errno), STDERR_FILENO);
			ft_putstr_fd("\n", STDERR_FILENO);
		}
		else
			data->status = run_builtin(exec->args, &(data->env),
					data->status, &(data->should_exit));
		clean_exit(data);
		close(STDOUT_FILENO);
		close_fds(exec->in_fd, exec->out_fd);
		exit(data->status);
	}
	if (pid == -1)
		set_syscall_err(&(data->err));
	else
		set_last_pid(data, pid);
}

void	exec_builtin(t_exec *exec, t_data *data)
{
	int	fd;

	if (!is_one_cmd(data))
		return (fork_builtin(exec, data),
			close_fds(exec->in_fd, exec->out_fd));
	fd = dup(STDOUT_FILENO);
	if (fd == -1)
		return (close_fds(exec->in_fd, exec->out_fd),
			set_syscall_err(&(data->err)));
	if (exec->out_fd >= 0 && dup2(exec->out_fd, STDOUT_FILENO) == -1)
		return (close_fds(exec->in_fd, exec->out_fd),
			close(fd), set_syscall_err(&(data->err)));
	data->status = run_builtin(exec->args, &(data->env),
			data->status, &(data->should_exit));
	if (dup2(fd, STDOUT_FILENO) == -1)
		return (close(fd), set_syscall_err(&(data->err)));
	close(fd);
	close_fds(exec->in_fd, exec->out_fd);
}
