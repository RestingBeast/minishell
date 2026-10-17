/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   exec_redir.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kkhant-z <kkhant-z@student.42singapore.sg  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/17 22:33:39 by kkhant-z          #+#    #+#             */
/*   Updated: 2026/10/17 22:33:41 by kkhant-z         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "minishell.h"

static int	get_flags(t_type type)
{
	if (type == REDIR_IN)
		return (O_RDONLY);
	if (type == REDIR_OUT)
		return (O_CREAT | O_TRUNC | O_WRONLY);
	if (type == APPEND)
		return (O_CREAT | O_APPEND | O_WRONLY);
	return (0);
}

static void	set_fds(t_type type, t_exec *exec, int fd)
{
	if (type == REDIR_IN || type == HEREDOC)
	{
		close_fds(exec->in_fd, -1);
		exec->in_fd = fd;
	}
	else
	{
		close_fds(-1, exec->out_fd);
		exec->out_fd = fd;
	}
}

static	int	copy_heredoc_fd(t_redir *rdr, t_exec *exec)
{
	int	fd;

	fd = dup(rdr->target.fd);
	if (fd == -1)
		return (0);
	set_fds(rdr->type, exec, fd);
	return (1);
}

int	resolve_redir(t_list *redir, t_err *err, t_exec *exec)
{
	t_redir	*rdr;
	int		fd;

	while (redir != NULL)
	{
		rdr = (t_redir *)(redir->content);
		if (rdr->ambiguous)
			return (close_fds(exec->in_fd, exec->out_fd),
				set_ambiguous_err(err, rdr->target.file), 0);
		if (rdr->type != HEREDOC)
		{
			fd = open(rdr->target.file, get_flags(rdr->type), 0666);
			if (fd == -1)
				return (set_file_err(err, rdr->target.file),
					close_fds(exec->in_fd, exec->out_fd), 0);
			set_fds(rdr->type, exec, fd);
		}
		else
			if (!copy_heredoc_fd(rdr, exec))
				return (close_fds(exec->in_fd, exec->out_fd),
					set_syscall_err(err), 0);
		redir = redir->next;
	}
	return (1);
}
