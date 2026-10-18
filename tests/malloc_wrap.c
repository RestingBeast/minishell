#include "test.h"

/*
** The test binary is linked with -Wl,--wrap=malloc, so every malloc in
** src/ and libft goes through here. fail_malloc_at(n) makes the n-th
** malloc from now return NULL. stop_failing() turns that off and returns
** how many mallocs were made, so a test knows when it has tried them all.
** Any test file can use them: they are declared in test.h.
*/
void	*__real_malloc(size_t size);

static int	g_fail_at;
static int	g_calls;

void	*__wrap_malloc(size_t size)
{
	g_calls++;
	if (g_fail_at != 0 && g_calls == g_fail_at)
		return (NULL);
	return (__real_malloc(size));
}

void	fail_malloc_at(int n)
{
	g_fail_at = n;
	g_calls = 0;
}

int	stop_failing(void)
{
	g_fail_at = 0;
	return (g_calls);
}
