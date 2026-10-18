#include "test.h"

static int	compare_lexemes(char *out, char *exp)
{
	if (out == NULL || exp == NULL)
		return (out == exp);
	return (strcmp(out, exp) == 0);
}

int	compare_tokens(t_list *out, t_list *exp)
{
	t_token	*t1;
	t_token	*t2;
	int		i;

	i = 0;
	while (out != NULL || exp != NULL)
	{
		if ((out && !exp) || (!out && exp))
			return (0);
		t1 = (t_token *)out->content;
		t2 = (t_token *)exp->content;
		if (t1->type != t2->type)
		{
			printf(
				"Token #%d\n"
				"    Expected type: %s\n"
				"    Actual type:   %s\n",
				i + 1,
				type_name(t2->type),
				type_name(t1->type)
			);
			return (0);
		}
		if (!compare_lexemes(t1->lexeme, t2->lexeme))
		{
			printf(
				"Token #%d\n"
				"    Expected lexeme: %s\n"
				"    Actual lexeme:   %s\n",
				i + 1,
				t2->lexeme ? t2->lexeme : "(null)",
				t1->lexeme ? t1->lexeme : "(null)"
			);
			return (0);
		}
		out = out->next;
		exp = exp->next;
		i++;
	}
	return (1);
}
