/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   envp.h                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: psoe <psoe@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/10 18:22:48 by psoe              #+#    #+#             */
/*   Updated: 2026/10/10 18:22:48 by psoe             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef ENVP_H
# define ENVP_H
# include "libft.h"

/* PATH given to a shell started without one, like env -i ./minishell. */
# define DEFAULT_PATH "/usr/local/bin:/usr/bin:/bin:/usr/sbin:/sbin"

/* Each node's content is one malloc'd string: "KEY=value", or "KEY". */

int		env_init(t_list **env, char **envp);
char	*env_get(t_list *env, char *key);
int		env_set(t_list **env, char *key, char *value);
void	env_unset(t_list **env, char *key);
t_list	*env_find(t_list *env, char *key);
int		env_print(t_list **env);
char	**env_to_array(t_list *env);
void	free_array(char **array);
int		update_shlvl(t_list **env);
int		update_pwd(t_list **env);
int		update_oldpwd(t_list **env);

#endif
