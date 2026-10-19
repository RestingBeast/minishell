/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_exit.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: psoe <psoe@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/12 10:36:10 by psoe              #+#    #+#             */
/*   Updated: 2026/10/12 19:29:02 by psoe             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "minishell.h"
#include <limits.h>

/*
** Reads ARG as a whole number, the way bash does for exit:
** spaces around it and one + or - sign are fine, and the number
** must fit in a long long. The digits go in *number and the sign
** in *negative. Returns 1 when ARG is a valid number, 0 when not.
*/
static int	read_number(char *arg, unsigned long long *number, int *negative)
{
	unsigned long long	limit;

	while (is_space(*arg))
		arg++;
	*negative = (*arg == '-');
	if (*arg == '-' || *arg == '+')
		arg++;
	if (!ft_isdigit(*arg))
		return (0);
	limit = LLONG_MAX;
	if (*negative)
		limit = limit + 1;
	*number = 0;
	while (ft_isdigit(*arg))
	{
		if (*number > (limit - (*arg - '0')) / 10)
			return (0);
		*number = *number * 10 + (*arg - '0');
		arg++;
	}
	while (is_space(*arg))
		arg++;
	return (*arg == '\0');
}

/*
** The exit builtin. It does not quit the program by itself: it sets
** *should_exit to 1 and returns the exit code, so the caller can free
** everything first and then call exit().
**   exit         -> last_status ($?)
**   exit 42      -> 42, and 256 -> 0, -1 -> 255 (the code is 0 to 255)
**   exit abc     -> error, and the shell exits with status 2
**   exit 1 2     -> error, returns 2 (like bash --posix) and the shell
**                   keeps running
** Like bash, "exit" is printed only when someone types the commands,
** and before any error: exec_builtin() prints it before calling us.
*/
int	builtin_exit(char **argv, int last_status, int *should_exit)
{
	unsigned long long	number;
	int					negative;
	int					code;

	*should_exit = 1;
	if (argv == NULL || argv[1] == NULL)
		return (last_status);
	if (!read_number(argv[1], &number, &negative))
	{
		print_error("exit", argv[1], "numeric argument required");
		return (2);
	}
	if (argv[2] != NULL)
	{
		*should_exit = 0;
		print_error("exit", NULL, "too many arguments");
		return (2);
	}
	code = number % 256;
	if (negative && code != 0)
		code = 256 - code;
	return (code);
}
