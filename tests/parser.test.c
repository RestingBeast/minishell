#include "test.h"
#include <criterion/criterion.h>
#include <criterion/redirect.h>

Test(Parser, WORD_test)
{
	char		*arg1[] = {"echo", "hell\'o wo\'\"rld\"", NULL};
	t_cmd		cmd1 = CMD(arg1, NULL);
	t_node		expected = NODE((void *) &cmd1, COMMAND, NULL, NULL);

	t_token	tok1 = TOK("echo", WORD);
	t_token	tok2 = TOK("hell\'o wo\'\"rld\"", WORD);

	t_list		lst = LST((void *) &tok2, NULL);
	t_list		tokens = LST((void *) &tok1, &lst);

	t_data	data;
	t_node		*root = parse_tokens(&tokens, &data);
	cr_assert_eq(data.err.type, OK);
	cr_assert(compare_trees(&expected, root));
	// ft_nodeclear(root);
}

Test(Parser, REDIR_test)
{
	char	*arg1[] = {"echo", "hello", NULL};
	t_redir	redir1 = REDIR(REDIR_OUT, (t_tgt){.file = "out"});
	t_list	redir_lst1 = LST((void *) &redir1, NULL);
	t_cmd	cmd1 = CMD(arg1, &redir_lst1);
	t_node	expected = NODE((void *) &cmd1, COMMAND, NULL, NULL);

	t_token	tok1 = TOK("echo", WORD);
	t_token	tok2 = TOK("hello", WORD);
	t_token	tok3 = TOK(NULL, REDIR_OUT);
	t_token	tok4 = TOK("out", WORD);

	t_list	lst3 = LST((void *)&tok4, NULL);
	t_list	lst2 = LST((void *)&tok3, &lst3);
	t_list	lst1 = LST((void *)&tok2, &lst2);
	t_list	tokens = LST((void *)&tok1, &lst1);

	t_data	data;
	t_node		*root = parse_tokens(&tokens, &data);
	cr_assert_eq(data.err.type, OK);
	cr_assert(compare_trees(&expected, root));
	// ft_nodeclear(root);
}

Test(Parser, HEREDOC_test)
{
	char	*arg[] = {"cat", NULL};
	t_redir	redir = REDIR(HEREDOC, (t_tgt){.fd = 2});
	t_list	r_lst = LST((void *)&redir, NULL);
	t_cmd	cmd = CMD(arg, &r_lst);
	t_node	expected = NODE((void *) &cmd, COMMAND, NULL, NULL);

	t_token	tok0 = TOK("cat", WORD);
	t_token	tok1 = TOK(NULL, HEREDOC);
	t_token	tok2 = TOK(ft_strdup("abc"), WORD);

	t_list	lst2 = LST((void *)&tok2, NULL);
	t_list	lst1 = LST((void *)&tok1, &lst2);
	t_list	tokens = LST((void *)&tok0, &lst1);

	cr_redirect_stdin();
	FILE *f_stdin = cr_get_redirected_stdin();
	fprintf(f_stdin, "abc\n");
	fclose(f_stdin);

	t_data	data;
	t_node		*root = parse_tokens(&tokens, &data);
	cr_assert_eq(data.err.type, OK);
	cr_assert(compare_trees(&expected, root));
	// ft_nodeclear(root);
}

Test(Parser, Multiple_REDIR_test)
{
	char	*arg1[] = {"echo", "hello", NULL};
	t_redir	redir1 = REDIR(REDIR_OUT, (t_tgt){.file = "out1"});
	t_redir	redir2 = REDIR(REDIR_OUT, (t_tgt){.file = "out2"});
	t_redir	redir3 = REDIR(REDIR_OUT, (t_tgt){.file = "out3"});

	t_list	r_lst3 = LST((void *) &redir3, NULL);
	t_list	r_lst2 = LST((void *) &redir2, &r_lst3);
	t_list	r_lst1 = LST((void *) &redir1, &r_lst2);

	t_cmd	cmd1 = CMD(arg1, &r_lst1);
	t_node	expected = NODE((void *) &cmd1, COMMAND, NULL, NULL);

	t_token	t0 = TOK("echo", WORD);
	t_token	t1 = TOK(NULL, REDIR_OUT);
	t_token	t2 = TOK("out1", WORD);
	t_token	t3 = TOK("hello", WORD);
	t_token	t4 = TOK(NULL, REDIR_OUT);
	t_token	t5 = TOK("out2", WORD);
	t_token	t6 = TOK(NULL, REDIR_OUT);
	t_token	t7 = TOK("out3", WORD);

	t_list	lst7 = LST((void *)&t7, NULL);
	t_list	lst6 = LST((void *)&t6, &lst7);
	t_list	lst5 = LST((void *)&t5, &lst6);
	t_list	lst4 = LST((void *)&t4, &lst5);
	t_list	lst3 = LST((void *)&t3, &lst4);
	t_list	lst2 = LST((void *)&t2, &lst3);
	t_list	lst1 = LST((void *)&t1, &lst2);
	t_list	tokens = LST((void *)&t0, &lst1);

	t_data	data;
	t_node		*root = parse_tokens(&tokens, &data);
	cr_assert_eq(data.err.type, OK);
	cr_assert(compare_trees(&expected, root));
	// ft_nodeclear(root);
}

Test(Parser, PIPE_test)
{
	char	*arg1[] = {"cat", "file1", NULL};
	t_cmd	cmd1 = CMD(arg1, NULL);
	t_node	n1 = NODE((void *)&cmd1, COMMAND, NULL, NULL);

	char	*arg2[] = {"grep", "192.168.0.1", NULL};
	t_redir	redir = REDIR(REDIR_IN, (t_tgt){.file = "log"});
	t_list	r_lst = LST((void *)&redir, NULL);
	t_cmd	cmd2 = CMD(arg2, &r_lst);
	t_node	n2 = NODE((void *)&cmd2, COMMAND, NULL, NULL);

	t_node	expected = NODE(NULL, PIPE, &n1, &n2);

	t_token	t0 = TOK("cat", WORD);
	t_token	t1 = TOK("file1", WORD);
	t_token	t2 = TOK(NULL, PIPE);
	t_token	t3 = TOK("grep", WORD);
	t_token	t4 = TOK("192.168.0.1", WORD);
	t_token	t5 = TOK(NULL, REDIR_IN);
	t_token	t6 = TOK("log", WORD);

	t_list	lst6 = LST((void *)&t6, NULL);
	t_list	lst5 = LST((void *)&t5, &lst6);
	t_list	lst4 = LST((void *)&t4, &lst5);
	t_list	lst3 = LST((void *)&t3, &lst4);
	t_list	lst2 = LST((void *)&t2, &lst3);
	t_list	lst1 = LST((void *)&t1, &lst2);
	t_list	tokens = LST((void *)&t0, &lst1);

	t_data	data;
	t_node		*root = parse_tokens(&tokens, &data);
	cr_assert_eq(data.err.type, OK);
	cr_assert(compare_trees(&expected, root));
	// ft_nodeclear(root);
}

// Tests by Claude
// Command: cat file1 | grep foo | wc -l
Test(Parser, TRIPLE_PIPE_test)
{
	char	*arg1[] = {"cat", "file1", NULL};
	t_cmd	cmd1 = CMD(arg1, NULL);
	t_node	n1 = NODE((void *)&cmd1, COMMAND, NULL, NULL);

	char	*arg2[] = {"grep", "foo", NULL};
	t_cmd	cmd2 = CMD(arg2, NULL);
	t_node	n2 = NODE((void *)&cmd2, COMMAND, NULL, NULL);

	char	*arg3[] = {"wc", "-l", NULL};
	t_cmd	cmd3 = CMD(arg3, NULL);
	t_node	n3 = NODE((void *)&cmd3, COMMAND, NULL, NULL);

	t_node	inner_pipe = NODE(NULL, PIPE, &n1, &n2);
	t_node	expected = NODE(NULL, PIPE, &inner_pipe, &n3);

	t_token	t0 = TOK("cat", WORD);
	t_token	t1 = TOK("file1", WORD);
	t_token	t2 = TOK(NULL, PIPE);
	t_token	t3 = TOK("grep", WORD);
	t_token	t4 = TOK("foo", WORD);
	t_token	t5 = TOK(NULL, PIPE);
	t_token	t6 = TOK("wc", WORD);
	t_token	t7 = TOK("-l", WORD);

	t_list	lst7 = LST((void *)&t7, NULL);
	t_list	lst6 = LST((void *)&t6, &lst7);
	t_list	lst5 = LST((void *)&t5, &lst6);
	t_list	lst4 = LST((void *)&t4, &lst5);
	t_list	lst3 = LST((void *)&t3, &lst4);
	t_list	lst2 = LST((void *)&t2, &lst3);
	t_list	lst1 = LST((void *)&t1, &lst2);
	t_list	tokens = LST((void *)&t0, &lst1);

	t_data	data;
	t_node		*root = parse_tokens(&tokens, &data);
	cr_assert_eq(data.err.type, OK);
	cr_assert(compare_trees(&expected, root));
	// ft_nodeclear(root);
}

// Command: cat << EOF > out
Test(Parser, HEREDOC_THEN_REDIR_OUT_test)
{
	char	*arg[] = {"cat", NULL};
	t_redir	redir1 = REDIR(HEREDOC, (t_tgt){.fd = 2});
	t_redir	redir2 = REDIR(REDIR_OUT, (t_tgt){.file = "out"});
	t_list	r_lst2 = LST((void *)&redir2, NULL);
	t_list	r_lst1 = LST((void *)&redir1, &r_lst2);
	t_cmd	cmd = CMD(arg, &r_lst1);
	t_node	expected = NODE((void *) &cmd, COMMAND, NULL, NULL);

	t_token	tok0 = TOK("cat", WORD);
	t_token	tok1 = TOK(NULL, HEREDOC);
	t_token	tok2 = TOK(ft_strdup("EOF"), WORD);
	t_token	tok3 = TOK(NULL, REDIR_OUT);
	t_token	tok4 = TOK("out", WORD);

	t_list	lst3 = LST((void *)&tok4, NULL);
	t_list	lst2 = LST((void *)&tok3, &lst3);
	t_list	lst1 = LST((void *)&tok2, &lst2);
	t_list	lst = LST((void*)&tok1, &lst1);
	t_list	tokens = LST((void *)&tok0, &lst);

	cr_redirect_stdin();
	FILE *f_stdin = cr_get_redirected_stdin();
	fprintf(f_stdin, "heredoc body line 1\nheredoc body line 2\nEOF\n");
	fclose(f_stdin);

	t_data	data;
	t_node		*root = parse_tokens(&tokens, &data);
	cr_assert_eq(data.err.type, OK);
	cr_assert(compare_trees(&expected, root));
	// ft_nodeclear(root);
}

// Command: cat < in1 | grep pattern > out1
Test(Parser, PIPE_WITH_REDIRS_BOTH_SIDES_test)
{
	char	*arg1[] = {"cat", NULL};
	t_redir	redir1 = REDIR(REDIR_IN, (t_tgt){.file = "in1"});
	t_list	r_lst1 = LST((void *)&redir1, NULL);
	t_cmd	cmd1 = CMD(arg1, &r_lst1);
	t_node	n1 = NODE((void *)&cmd1, COMMAND, NULL, NULL);

	char	*arg2[] = {"grep", "pattern", NULL};
	t_redir	redir2 = REDIR(REDIR_OUT, (t_tgt){.file = "out1"});
	t_list	r_lst2 = LST((void *)&redir2, NULL);
	t_cmd	cmd2 = CMD(arg2, &r_lst2);
	t_node	n2 = NODE((void *)&cmd2, COMMAND, NULL, NULL);

	t_node	expected = NODE(NULL, PIPE, &n1, &n2);

	t_token	t0 = TOK("cat", WORD);
	t_token	t1 = TOK(NULL, REDIR_IN);
	t_token	t2 = TOK("in1", WORD);
	t_token	t3 = TOK(NULL, PIPE);
	t_token	t4 = TOK("grep", WORD);
	t_token	t5 = TOK("pattern", WORD);
	t_token	t6 = TOK(NULL, REDIR_OUT);
	t_token	t7 = TOK("out1", WORD);

	t_list	lst7 = LST((void *)&t7, NULL);
	t_list	lst6 = LST((void *)&t6, &lst7);
	t_list	lst5 = LST((void *)&t5, &lst6);
	t_list	lst4 = LST((void *)&t4, &lst5);
	t_list	lst3 = LST((void *)&t3, &lst4);
	t_list	lst2 = LST((void *)&t2, &lst3);
	t_list	lst1 = LST((void *)&t1, &lst2);
	t_list	tokens = LST((void *)&t0, &lst1);

	t_data	data;
	t_node		*root = parse_tokens(&tokens, &data);
	cr_assert_eq(data.err.type, OK);
	cr_assert(compare_trees(&expected, root));
	// ft_nodeclear(root);
}

// Command: cat << A << B  (two heredocs on same command — only B's body matters
// for execution, but the parser must still build two HEREDOC redir nodes, and
// the reader must consume both delimited blocks from stdin in order)
Test(Parser, MULTIPLE_HEREDOC_test)
{
	char	*arg[] = {"cat", NULL};
	t_redir	redir1 = REDIR(HEREDOC, (t_tgt){.fd = 2});
	t_redir	redir2 = REDIR(HEREDOC, (t_tgt){.fd = 3});
	t_list	r_lst2 = LST((void *)&redir2, NULL);
	t_list	r_lst1 = LST((void *)&redir1, &r_lst2);
	t_cmd	cmd = CMD(arg, &r_lst1);
	t_node	expected = NODE((void *) &cmd, COMMAND, NULL, NULL);

	t_token	tok0 = TOK("cat", WORD);
	t_token	tok1 = TOK(NULL, HEREDOC);
	t_token	tok2 = TOK(ft_strdup("A"), WORD);
	t_token	tok3 = TOK(NULL, HEREDOC);
	t_token	tok4 = TOK(ft_strdup("B"), WORD);

	t_list	lst3 = LST((void *)&tok4, NULL);
	t_list	lst2 = LST((void *)&tok3, &lst3);
	t_list	lst1 = LST((void *)&tok2, &lst2);
	t_list	lst0 = LST((void *)&tok1, &lst1);
	t_list	tokens = LST((void *)&tok0, &lst0);

	cr_redirect_stdin();
	FILE *f_stdin = cr_get_redirected_stdin();
	fprintf(f_stdin, "first block\nA\nsecond block\nB\n");
	fclose(f_stdin);

	t_data	data;
	t_node		*root = parse_tokens(&tokens, &data);
	cr_assert_eq(data.err.type, OK);
	cr_assert(compare_trees(&expected, root));
	// ft_nodeclear(root);
}

// Command: < input.txt cat | grep -v skip | sort >> final.txt
Test(Parser, FULL_PIPELINE_WITH_REDIRS_test)
{
	char	*arg1[] = {"cat", NULL};
	t_redir	redir1 = REDIR(REDIR_IN, (t_tgt){.file = "input.txt"});
	t_list	r_lst1 = LST((void *)&redir1, NULL);
	t_cmd	cmd1 = CMD(arg1, &r_lst1);
	t_node	n1 = NODE((void *)&cmd1, COMMAND, NULL, NULL);

	char	*arg2[] = {"grep", "-v", "skip", NULL};
	t_cmd	cmd2 = CMD(arg2, NULL);
	t_node	n2 = NODE((void *)&cmd2, COMMAND, NULL, NULL);

	char	*arg3[] = {"sort", NULL};
	t_redir	redir3 = REDIR(APPEND, (t_tgt){.file = "final.txt"});
	t_list	r_lst3 = LST((void *)&redir3, NULL);
	t_cmd	cmd3 = CMD(arg3, &r_lst3);
	t_node	n3 = NODE((void *)&cmd3, COMMAND, NULL, NULL);

	t_node	inner_pipe = NODE(NULL, PIPE, &n1, &n2);
	t_node	expected = NODE(NULL, PIPE, &inner_pipe, &n3);

	t_token	t0 = TOK(NULL, REDIR_IN);
	t_token	t1 = TOK("input.txt", WORD);
	t_token	t2 = TOK("cat", WORD);
	t_token	t3 = TOK(NULL, PIPE);
	t_token	t4 = TOK("grep", WORD);
	t_token	t5 = TOK("-v", WORD);
	t_token	t6 = TOK("skip", WORD);
	t_token	t7 = TOK(NULL, PIPE);
	t_token	t8 = TOK("sort", WORD);
	t_token	t9 = TOK(NULL, APPEND);
	t_token	t10 = TOK("final.txt", WORD);

	t_list	lst10 = LST((void *)&t10, NULL);
	t_list	lst9 = LST((void *)&t9, &lst10);
	t_list	lst8 = LST((void *)&t8, &lst9);
	t_list	lst7 = LST((void *)&t7, &lst8);
	t_list	lst6 = LST((void *)&t6, &lst7);
	t_list	lst5 = LST((void *)&t5, &lst6);
	t_list	lst4 = LST((void *)&t4, &lst5);
	t_list	lst3 = LST((void *)&t3, &lst4);
	t_list	lst2 = LST((void *)&t2, &lst3);
	t_list	lst1 = LST((void *)&t1, &lst2);
	t_list	tokens = LST((void *)&t0, &lst1);

	t_data	data;
	t_node		*root = parse_tokens(&tokens, &data);
	cr_assert_eq(data.err.type, OK);
	cr_assert(compare_trees(&expected, root));
	// ft_nodeclear(root);
}
