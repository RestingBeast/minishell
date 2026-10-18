/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   minishell.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: psoe <psoe@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/07 20:44:16 by kkhant-z          #+#    #+#             */
/*   Updated: 2026/08/12 22:54:58 by psoe             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "minishell.h"

static int	init_envlst(t_list **env)
{
	extern char	**environ;

	if (env_init(env, environ) == -1)
	{
		ft_putendl_fd("minishell: malloc: failed to allocate memory",
			STDERR_FILENO);
		return (1);
	}
	return (0);
}

/*
** Ctrl-D at the prompt or the exit builtin. Like bash, "exit" is
** printed only when someone is typing (stdin is a terminal), and on stderr.
** After the exit builtin it was already printed by exec_builtin(),
** before any error from exit, so here it is only printed for Ctrl-D.
** stdin and stdout are closed so valgrind --track-fds does not report
** stdout after a builtin redirection restored it with dup2().
** stderr stays open: ASan prints its leak report there at exit.
*/
static void	end_shell(t_data *data)
{
	if (!data->should_exit && isatty(STDIN_FILENO))
		ft_putendl_fd("exit", STDERR_FILENO);
	clean_exit(data);
	close(STDIN_FILENO);
	close(STDOUT_FILENO);
}

/*
** Lexes, parses, expands and runs one line.
** Returns 0 when the exit builtin says the shell must stop.
*/
static int	run_line(t_data *data)
{
	t_exec	exec;

	data->tokens = lexer(data->line, &(data->err));
	if (!check_error(data))
		return (1);
	data->tree = parse_tokens(data->tokens, data);
	if (!check_error(data))
		return (1);
	expand_tree(data->tree, data->env, data->status, &(data->err));
	if (!check_error(data))
		return (1);
	init_exec(&exec);
	execute_tree(data->tree, data, &exec, 0);
	wait_children(data->last_pid, &(data->status), data->processes);
	if (data->should_exit)
		return (clean_up(data), 0);
	if (!check_error(data))
		return (1);
	clean_up(data);
	return (1);
}

int	main(void)
{
	t_data	data;

	if (init_envlst(&(data.env)) == 1)
		return (1);
	data.status = 0;
	data.should_exit = 0;
	while (1)
	{
		init_shell(&data);
		data.line = readline("minishell$ ");
		if (g_sig_captured == SIGINT)
		{
			data.status = 130;
			g_sig_captured = 0;
		}
		if (!data.line)
			break ;
		if (*(data.line))
			add_history(data.line);
		if (!run_line(&data))
			break ;
	}
	end_shell(&data);
	return (data.status);
}
