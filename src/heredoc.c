/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   heredoc.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: psoe <psoe@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/13 14:28:15 by psoe              #+#    #+#             */
/*   Updated: 2026/10/13 14:28:15 by psoe             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "minishell.h"

static void	expand_and_write(char *line, int fd, t_data *data, int has_quotes)
{
	int		i;
	char	*tmp;

	i = -1;
	while (line[++i] != '\0')
	{
		if (!has_quotes && line[i] == '$')
		{
			tmp = expand_dollar(line, &i, data->env, data->status);
			if (!tmp)
				return (set_malloc_err(&(data->err)));
			write(fd, tmp, ft_strlen(tmp));
			free(tmp);
			i--;
		}
		else
			write(fd, &(line[i]), 1);
	}
	write(fd, "\n", 1);
}

static void	capture_input(char *delimiter, int fd, t_data *data, int has_quotes)
{
	char	*line;

	g_sig_captured = 0;
	set_heredoc_sig();
	if (isatty(STDIN_FILENO))
		rl_event_hook = rl_check;
	line = readline("> ");
	while (line != NULL && g_sig_captured == 0)
	{
		if (ft_memcmp((void *)delimiter,
				(void *)line, ft_strlen(delimiter) + 1) == 0)
			break ;
		expand_and_write(line, fd, data, has_quotes);
		free(line);
		line = readline("> ");
	}
	if (g_sig_captured == SIGINT)
		data->err.type = HEREDOC_ABORT;
	if (line == NULL)
		warn_eof(delimiter);
	free(line);
	rl_event_hook = NULL;
	g_sig_captured = 0;
}

/*
** Creates a new empty file in /tmp and opens it for writing.
** O_EXCL makes open() fail when the name is already taken,
** so we try _0, _1, _2... until one is free.
*/
static int	open_tmp_file(char **path, t_err *err)
{
	int	fd;
	int	n;

	n = 0;
	while (1)
	{
		*path = make_tmp_path(n);
		if (!*path)
			return (set_malloc_err(err), -1);
		fd = open(*path, O_WRONLY | O_CREAT | O_EXCL, 0600);
		if (fd != -1)
			return (fd);
		free(*path);
		*path = NULL;
		if (errno != EEXIST)
			return (set_syscall_err(err), -1);
		n++;
	}
}

/*
** The heredoc text goes into a temp file, not a pipe: a pipe holds
** only 64 KB, and nobody reads it before the command runs.
** We write the file, then open it again to read from the start.
** unlink() removes the name right away, but our fd can still read it.
** The file is really deleted when the fd is closed.
** Returns that fd, or -1 and sets the error in DATA.
*/
static int	read_heredoc(char *delimiter, t_data *data, int has_quotes)
{
	char	*path;
	int		fd;

	fd = open_tmp_file(&path, &(data->err));
	if (fd == -1)
		return (-1);
	capture_input(delimiter, fd, data, has_quotes);
	close(fd);
	fd = open(path, O_RDONLY);
	unlink(path);
	free(path);
	if (fd == -1)
		return (set_syscall_err(&(data->err)), -1);
	return (fd);
}

t_redir	*handle_heredoc(t_list *tok, t_data *data)
{
	t_redir	*res;
	char	*delimiter;
	int		fd;
	int		has_quotes;

	has_quotes = 0;
	if (ft_strchr(((t_token *)tok->next->content)->lexeme, '\'')
		|| ft_strchr(((t_token *)tok->next->content)->lexeme, '\"'))
		has_quotes = 1;
	delimiter = strip_quotes(((t_token *)tok->next->content)->lexeme);
	if (!delimiter)
		return (set_malloc_err(&(data->err)), NULL);
	((t_token *)tok->next->content)->lexeme = NULL;
	fd = read_heredoc(delimiter, data, has_quotes);
	free(delimiter);
	if (fd == -1)
		return (NULL);
	res = ft_redirnew_fd(HEREDOC, fd);
	if (!res)
		return (close(fd), set_malloc_err(&(data->err)), NULL);
	return (res);
}
