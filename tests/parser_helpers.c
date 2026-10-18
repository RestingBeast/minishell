#include "test.h"

static const char	*safe_arr(char **arr)
{
	return ((arr && *arr) ? *arr : "(null)");
}

static const char	*safe_str(char *str)
{
	return (str ? str : "(null)");
}

static const char	*safe_redir(t_list *lst)
{
	return (lst ? type_name(((t_redir *)lst->content)->type) : "(null)");
}

static const char	*safe_cmd(t_cmd *cmd)
{
	return (cmd ? safe_arr(cmd->args) : "(null)");
}

static const char	*safe_node(t_node *node)
{
	return (node ? type_name(node->type) : "(null)");
}

static int	compare_arguments(char **arg1, char **arg2, int *index)
{
	int	i;
	int	len;

	i = 0;
	if (!arg1 || !arg2)
	{
		printf(
			"Node #%d\n"
			"Expected arguments: %s\n"
			"Actual arguments:   %s\n",
			*index,
			safe_arr(arg1),
			safe_arr(arg2)
		);
		return (0);
	}
	while (arg1[i] != NULL && arg2[i] != NULL)
	{
		if (ft_strlen(arg1[i]) > ft_strlen(arg2[i]))
			len = ft_strlen(arg1[i]);
		else
			len = ft_strlen(arg2[i]);
		if (ft_strncmp(arg1[i], arg2[i], len) != 0)
		{
			printf(
				"Node #%d\n"
				"Expected arguement[%d]: %s\n"
				"Actual arguement[%d]:   %s\n",
				*index,
				i,
				arg1[i],
				i,
				arg2[i]
			);
			return (0);
		}
		i++;
	}
	if (arg1[i] != NULL || arg2[i] != NULL)
	{
		printf(
			"Node #%d\n"
			"Expected arguement[%d]: %s\n"
			"Actual arguement[%d]:   %s\n",
			*index,
			i,
			safe_str(arg1[i]),
			i,
			safe_str(arg2[i])
		);
		return (0);
	}
	return (1);
}

static int	compare_redirections(t_list *lst1, t_list *lst2, int *index)
{
	t_redir	*rdr1;
	t_redir	*rdr2;
	char	*s1;
	char	*s2;
	int		len;

	while (lst1 != NULL && lst2 != NULL)
	{
		rdr1 = (t_redir *)lst1->content;
		rdr2 = (t_redir *)lst2->content;
		if (rdr1->type != rdr2->type)
		{
			printf(
				"Node #%d\n"
				"Expected type: %s\n"
				"Actual type:   %s\n",
				*index,
				type_name(rdr1->type),
				type_name(rdr2->type)
			);
			return (0);
		}
		if (rdr1->type != HEREDOC)
		{
			s1 = (char *)rdr1->target.file;
			s2 = (char *)rdr2->target.file;
			if (s1 == NULL || s2 == NULL)
			{
				printf(
					"Node #%d\n"
					"Expected target: %s\n"
					"Actual target:   %s\n",
					*index,
					safe_str(s1),
					safe_str(s2)
				);
				return (0);
			}
			if (ft_strlen(s1) > ft_strlen(s2))
				len = ft_strlen(s1);
			else
				len = ft_strlen(s2);
			if (ft_strncmp(s1, s2, len) != 0)
			{	
				printf(
					"Node #%d\n"
					"Expected target: %s\n"
					"Actual target:   %s\n",
					*index,
					safe_str(s1),
					safe_str(s2)
				);
				return (0);
			}
		}
		lst1 = lst1->next;
		lst2 = lst2->next;
	}
	if (lst1 != NULL || lst2 != NULL)
	{
		printf(
			"NODE #%d\n"
			"Expected type: %s\n"
			"Actual type: %s\n",
			*index,
			safe_redir(lst1),
			safe_redir(lst2)
		);
		return (0);
	}
	return (1);
}

static int	compare_command(t_node *node1, t_node *node2, int *index)
{
	t_cmd	*cmd1;
	t_cmd	*cmd2;

	cmd1 = (t_cmd *) node1->data;
	cmd2 = (t_cmd *) node2->data;
	if (!cmd1 || !cmd2)
	{
		printf(
			"Node #%d\n"
			"Expected type: %s\n"
			"Actual type:   %s\n",
			*index,
			safe_cmd(cmd1),
			safe_cmd(cmd2)
		);
		return (0);
	}
	if (cmd1 != NULL && cmd2 != NULL)
	{
		if (compare_arguments(cmd1->args, cmd2->args, index) == 0)
			return (0);
		if (compare_redirections(cmd1->redir, cmd2->redir, index) == 0)
			return (0);
	}
	return (1);
}

static int	compare_node(t_node *node1, t_node *node2, int *index)
{
	if ((node1 && !node2) || (!node1 && node2))
		return (0);
	if (node1 != NULL && node2 != NULL)
	{
		(*index)++;
		if (node1->type != node2->type)
		{
			printf(
				"Node #%d\n"
				"Expected type: %s\n"
				"Actual type:   %s\n",
				*index,
				type_name(node1->type),
				type_name(node2->type)
			);
			return (0);
		}
		else if (node1->type == COMMAND)
		{
			if (!compare_command(node1, node2, index))
				return (0);
		}
	}
	return (1);
}

int compare_trees(t_node *node1, t_node *node2)
{
	t_node	*curr1 = NULL;
	t_node	*curr2 = NULL;
	int		index;
	int		res;

	res = 1;
	index = 0;
	while (node1 || node2)
	{
		if ((node1 && !node2) || (!node1 && node2))
		{
			printf(
				"Node #%d\n"
				"Expected type: %s\n"
				"Actual type:   %s\n",
				index,
				safe_node(node1),
				safe_node(node2)
			);
			return (0);
		}
		node1 = morris_next(node1, &curr1);
		node2 = morris_next(node2, &curr2);
		if (compare_node(curr1, curr2, &index) == 0)
			res = 0;
	}
	return (res);
}

static t_node	*morris_link_pred(t_node *node, t_node **curr)
{
	t_node	*pred;

	pred = node->left;
	while (pred->right != NULL && pred->right != node)
		pred = pred->right;
	if (pred->right == NULL)
	{
		pred->right = node;
		*curr = NULL;
		return (node->left);
	}
	else
	{
		pred->right = NULL;
		*curr = node;
		return (node->right);
	}
}

t_node	*morris_next(t_node *node, t_node **curr)
{
	if (!node)
		return (NULL);
	if (node->left == NULL)
	{
		*curr = node;
		return (node->right);
	}
	else
		return (morris_link_pred(node, curr));
}
