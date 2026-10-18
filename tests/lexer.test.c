#include "test.h"
#include <criterion/criterion.h>

// Command: echo "Hello, World"
Test(Lexer, basic_test)
{
	t_token tok1 = TOK("echo", WORD);
	t_token tok2 = TOK("\"Hello, World\"", WORD);

	t_list lst2 = LST((void *) &tok2, NULL);
	t_list expected = LST((void *) &tok1, &lst2);

	t_err	err;
	t_list *tokens = lexer("echo \"Hello, World\"", &err);

	cr_assert_eq(err.type, OK);
	cr_assert(compare_tokens(tokens, &expected));
	free_tokens(tokens);
}

// Command: echo 'He"llo Wo"rld'
Test(Lexer, single_quoted_word)
{
	t_token tok1 = TOK("echo", WORD);
	t_token tok2 = TOK("'He\"llo Wo\"rld'", WORD);

	t_list lst2 = LST((void *) &tok2, NULL);
	t_list expected = LST((void *) &tok1, &lst2);

	t_err	err;
	t_list *tokens = lexer("echo 'He\"llo Wo\"rld'", &err);

	cr_assert_eq(err.type, OK);
	cr_assert(compare_tokens(tokens, &expected));
	free_tokens(tokens);
}

// Command: echo He"llo" Wo'rld'
Test(Lexer, mixed_quotes)
{
	t_token tok1 = TOK("echo", WORD);
	t_token tok2 = TOK("He\"llo\"", WORD);
	t_token tok3 = TOK("Wo'rld'", WORD);

	t_list lst3 = LST((void *) &tok3, NULL);
	t_list lst2 = LST((void *) &tok2, &lst3);
	t_list expected = LST((void *) &tok1, &lst2);

	t_err	err;
	t_list *tokens = lexer("echo He\"llo\" Wo'rld'", &err);

	cr_assert_eq(err.type, OK);
	cr_assert(compare_tokens(tokens, &expected));
	free_tokens(tokens);
}

// Command: echo Hello World | grep "Hello"
Test(Lexer, pipe_and_quotes)
{
	t_token tok1 = TOK("echo", WORD);
	t_token tok2 = TOK("Hello", WORD);
	t_token tok3 = TOK("World", WORD);
	t_token tok4 = TOK(NULL, PIPE);
	t_token tok5 = TOK("grep", WORD);
	t_token tok6 = TOK("\"Hello\"", WORD);

	t_list lst6 = LST((void *) &tok6, NULL);
	t_list lst5 = LST((void *) &tok5, &lst6);
	t_list lst4 = LST((void *) &tok4, &lst5);
	t_list lst3 = LST((void *) &tok3, &lst4);
	t_list lst2 = LST((void *) &tok2, &lst3);
	t_list expected = LST((void *) &tok1, &lst2);

	t_err	err;
	t_list *tokens = lexer("echo Hello World | grep \"Hello\"", &err);

	cr_assert_eq(err.type, OK);
	cr_assert(compare_tokens(tokens, &expected));
	free_tokens(tokens);
}

// Command: echo Hello >text
Test(Lexer, output_redirect_no_space)
{
	t_token tok1 = TOK("echo", WORD);
	t_token tok2 = TOK("Hello", WORD);
	t_token tok3 = TOK(NULL, REDIR_OUT);
	t_token tok4 = TOK("text", WORD);

	t_list lst4 = LST((void *) &tok4, NULL);
	t_list lst3 = LST((void *) &tok3, &lst4);
	t_list lst2 = LST((void *) &tok2, &lst3);
	t_list expected = LST((void *) &tok1, &lst2);

	t_err	err;
	t_list *tokens = lexer("echo Hello >text", &err);

	cr_assert_eq(err.type, OK);
	cr_assert(compare_tokens(tokens, &expected));
	free_tokens(tokens);
}

// Command: < "$FILE" cat
Test(Lexer, input_redirect_space)
{
	t_token tok1 = TOK(NULL, REDIR_IN);
	t_token tok2 = TOK("\"$FILE\"", WORD);
	t_token tok3 = TOK("cat", WORD);

	t_list lst3 = LST((void *) &tok3, NULL);
	t_list lst2 = LST((void *) &tok2, &lst3);
	t_list expected = LST((void *) &tok1, &lst2);

	t_err	err;
	t_list *tokens = lexer("< \"$FILE\" cat", &err);

	cr_assert_eq(err.type, OK);
	cr_assert(compare_tokens(tokens, &expected));
	free_tokens(tokens);
}

// Command: <<'EO F' cat
Test(Lexer, heredoc_before_command)
{
	t_token tok1 = TOK(NULL, HEREDOC);
	t_token tok2 = TOK("'EO F'", WORD);
	t_token tok3 = TOK("cat", WORD);

	t_list lst3 = LST((void *) &tok3, NULL);
	t_list lst2 = LST((void *) &tok2, &lst3);
	t_list expected = LST((void *) &tok1, &lst2);

	t_err	err;
	t_list *tokens = lexer("<<\'EO F\' cat", &err);

	cr_assert_eq(err.type, OK);
	cr_assert(compare_tokens(tokens, &expected));
	free_tokens(tokens);
}

// Command: echo 'World!'>>"$OUT"
Test(Lexer, append_after_command)
{
	t_token tok1 = TOK("echo", WORD);
	t_token tok2 = TOK("'World!'", WORD);
	t_token tok3 = TOK(NULL, APPEND);
	t_token tok4 = TOK("\"$OUT\"", WORD);

	t_list lst4 = LST((void *) &tok4, NULL);
	t_list lst3 = LST((void *) &tok3, &lst4);
	t_list lst2 = LST((void *) &tok2, &lst3);
	t_list expected = LST((void *) &tok1, &lst2);

	t_err	err;
	t_list *tokens = lexer("echo \'World!\'>>\"$OUT\"", &err);

	cr_assert_eq(err.type, OK);
	cr_assert(compare_tokens(tokens, &expected));
	free_tokens(tokens);
}

// Command: echo Hell"o wo"'rld'
Test(Lexer, adjacent_quotes_with_space)
{
	t_token tok1 = TOK("echo", WORD);
	t_token tok2 = TOK("Hell\"o wo\"'rld'", WORD);

	t_list lst2 = LST((void *) &tok2, NULL);
	t_list expected = LST((void *) &tok1, &lst2);

	t_err	err;
	t_list *tokens = lexer("echo Hell\"o wo\"'rld'", &err);

	cr_assert_eq(err.type, OK);
	cr_assert(compare_tokens(tokens, &expected));
	free_tokens(tokens);
}

// Command: echo "unclosed
Test(Lexer, unclosed_double_quote)
{
	t_err	err;
	t_list *tokens = lexer("echo \"unclosed", &err);

	cr_assert_null(tokens);
	cr_assert_eq(err.type, UNCLOSED_QUOTE);
}

// Command: echo 'it"s | cat
Test(Lexer, unclosed_single_quote)
{
	t_err	err;
	t_list *tokens = lexer("echo 'it\"s | cat", &err);

	cr_assert_null(tokens);
	cr_assert_eq(err.type, UNCLOSED_QUOTE);
}

// Command: (empty and whitespace-only lines)
Test(Lexer, empty_input)
{
	t_err	err;

	cr_assert_null(lexer("", &err));
	cr_assert_eq(err.type, OK);
	// cr_assert_null(lexer("  \t ", &err));
	// cr_assert_eq(err.type, OK);
}

// Command: echo |
Test(Lexer, end_pipe)
{
	t_err	err;
	t_list	*tokens = lexer("echo |", &err);
	
	cr_assert_null(tokens);
	cr_assert_eq(err.type, END_PIPE);
}

// Command: echo >
Test(Lexer, end_redir)
{
	t_err	err;
	t_list	*tokens = lexer("echo >", &err);
	
	cr_assert_null(tokens);
	cr_assert_eq(err.type, END_REDIR);
}

// Command: cat >>> file
Test(Lexer, end_unexp_tok)
{
	t_err	err;
	t_list	*tokens = lexer("cat >>> file", &err);
	
	cr_assert_null(tokens);
	cr_assert_eq(err.type, UNEXPECTED_TOK);
}
