/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   errors.h                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kkhant-z <kkhant-z@student.42singapore.sg  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/30 18:01:25 by kkhant-z          #+#    #+#             */
/*   Updated: 2026/09/30 18:01:27 by kkhant-z         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef ERROR_H
# define ERROR_H

typedef struct s_data	t_data;
typedef enum e_err_type	t_err_type;
typedef union u_payload	t_payload;
typedef struct s_err	t_err;

enum	e_err_type
{
	OK = 0,
	MALLOC_ERROR,
	UNCLOSED_QUOTE,
	UNEXPECTED_TOK,
	END_PIPE,
	END_REDIR,
	HEREDOC_ABORT,
	SYSCALL_ERR,
	CMD_ERROR,
	CMD_NOT_FOUND,
	IS_DIR_ERR,
	AMBIGUOUS_ERR,
	FILE_ERR,
};

struct	s_err
{
	int			err_no;
	t_err_type	type;
	char		*msg;
};

// errors.c
int		get_status(t_err_type type);
int		check_error(t_data *data);
void	report_exec_err(t_data *data);

// syscall_err.c
void	set_malloc_err(t_err *err);
void	set_syscall_err(t_err *err);
void	set_ambiguous_err(t_err *err, char *file);
void	set_file_err(t_err *err, char *file);

// syntac_err.c
void	set_quote_err(t_err *err, char quote);
void	set_unexp_tok_err(t_err *err, t_type tok);
void	set_end_pipe_err(t_err *err);
void	set_end_redir_err(t_err *err);

// cmd_err.c
void	print_cmd_err_msg(t_err_type type, int err_no, char *msg);
void	set_cmd_err(t_err *err, char *cmd);
void	set_cmd_not_found_err(t_err *err, char *cmd);
void	set_dir_err(t_err *err, char *cmd);

#endif
