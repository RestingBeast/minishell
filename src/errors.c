/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   errors.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kkhant-z <kkhant-z@student.42singapore.sg  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/30 18:31:20 by kkhant-z          #+#    #+#             */
/*   Updated: 2026/09/30 18:31:21 by kkhant-z         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "minishell.h"

/*
** Redirection errors name the file, like bash:
** "minishell: nofile: No such file or directory"
*/
static void	print_redir_err(t_err *err)
{
	ft_putstr_fd(err->msg, STDERR_FILENO);
	ft_putstr_fd(": ", STDERR_FILENO);
	if (err->type == AMBIGUOUS_ERR)
		ft_putendl_fd("ambiguous redirect", STDERR_FILENO);
	else
		ft_putendl_fd(strerror(err->err_no), STDERR_FILENO);
}

static void	print_err_msg(t_err *err)
{
	ft_putstr_fd("minishell: ", STDERR_FILENO);
	if (err->type == UNEXPECTED_TOK)
	{
		ft_putstr_fd("syntax error near unexpected token `", STDERR_FILENO);
		ft_putstr_fd(err->msg, STDERR_FILENO);
		ft_putstr_fd("\'\n", STDERR_FILENO);
		return ;
	}
	if (err->type == CMD_ERROR || err->type == CMD_NOT_FOUND
		|| err->type == IS_DIR_ERR)
	{
		print_cmd_err_msg(err->type, err->err_no, err->msg);
		return ;
	}
	if (err->type == AMBIGUOUS_ERR || err->type == FILE_ERR)
	{
		print_redir_err(err);
		return ;
	}
	ft_putstr_fd(err->msg, STDERR_FILENO);
	if (err->type == SYSCALL_ERR)
		ft_putstr_fd(strerror(err->err_no), STDERR_FILENO);
	ft_putstr_fd("\n", STDERR_FILENO);
}

int	get_status(t_err_type type)
{
	if (type == MALLOC_ERROR || type == SYSCALL_ERR || type == AMBIGUOUS_ERR
		|| type == FILE_ERR)
		return (1);
	if (type == UNCLOSED_QUOTE || type == UNEXPECTED_TOK || type == END_PIPE
		|| type == END_REDIR)
		return (2);
	if (type == CMD_ERROR || type == IS_DIR_ERR)
		return (126);
	if (type == CMD_NOT_FOUND)
		return (127);
	if (type == HEREDOC_ABORT)
		return (130);
	return (0);
}

/*
** Reports the error of one command right away, so the next command of
** the line starts clean and $? ends up as the status of the last one.
** A malloc failure still ends the shell, like in check_error().
*/
void	report_exec_err(t_data *data)
{
	if (data->err.type == OK)
		return ;
	data->status = get_status(data->err.type);
	print_err_msg(&(data->err));
	if (data->err.type == MALLOC_ERROR)
	{
		clean_exit(data);
		exit(1);
	}
	data->err.type = OK;
	data->err.err_no = 0;
	data->err.msg = "";
}

int	check_error(t_data *data)
{
	if (data->err.type == OK)
		return (1);
	data->status = get_status(data->err.type);
	if (data->err.type == HEREDOC_ABORT)
		return (clean_up(data), 0);
	print_err_msg(&(data->err));
	clean_up(data);
	if (data->err.type == MALLOC_ERROR)
	{
		clean_exit(data);
		exit(1);
	}
	return (0);
}
