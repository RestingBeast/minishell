#include "test.h"

const char	*type_name(t_type type)
{
	switch (type)
	{
		case WORD: return ("WORD");
		case COMMAND: return ("COMMAND");
		case PIPE: return ("PIPE");
		case REDIR_IN: return ("REDIR_IN");
		case REDIR_OUT: return ("REDIR_OUT");
		case HEREDOC: return ("HEREDOC");
		case APPEND: return ("APPEND");
	}
	return (NULL);
}
