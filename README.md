*This project has been created as part of the 42 curriculum by kkhant-z, psoe.*

# minishell

## Description

minishell is a small shell written in C, modelled on bash. It reads a line
with [GNU Readline](https://tiswww.case.edu/php/chet/readline/rltop.html),
turns it into a tree of commands, and runs it.

It supports:

- **Commands** found through `PATH`, or given as a relative or absolute path.
  Started without `PATH` (`env -i ./minishell`), it uses a default one
  (`/usr/local/bin:/usr/bin:/bin:/usr/sbin:/sbin`); unlike bash, it is exported
- **Quotes**: `'...'` keeps everything as typed, `"..."` still expands `$`
- **Variables**: `$NAME` and `$?` (exit status of the last command)
- **Redirections**: `<`, `>`, `>>`, and `<<` (heredoc)
- **Heredocs** expand variables, unless the delimiter is quoted (`<< 'EOF'`)
- **Pipes**: `cmd1 | cmd2 | ...`, with all commands running at the same time
- **Builtins**: `echo` (with `-n`), `cd` (also `cd -`), `pwd`, `export`,
  `unset`, `env`, `exit`
- **Signals**: `Ctrl-C` shows a new prompt, `Ctrl-D` exits, `Ctrl-\` does nothing
  at the prompt; while a command runs they behave like in bash
- **Exit statuses** like bash: `127` command not found, `126` not executable,
  `2` syntax error, `128 + n` killed by signal `n`, ...

It does not interpret `\`, `;`, `&&`, `||`, parentheses or `*`, and it does not
split unquoted variables on spaces.

## Instructions

### Requirements

- `cc` and `make`
- GNU Readline development files (`libreadline-dev` on Debian/Ubuntu)

### Build and run

```sh
make            # builds libft, then ./minishell
./minishell
```

Other rules: `make clean` (object files), `make fclean` (objects and binary),
`make re` (rebuild from scratch).

### Check for leaks

`readline.supp` hides the memory that Readline itself keeps until exit:

```sh
valgrind --suppressions=readline.supp --leak-check=full --show-leak-kinds=all \
	--track-fds=yes --trace-children=yes ./minishell
```

## How it works

Each line goes through four steps, all driven by `run_line()` in
`src/minishell.c`:

1. **Lexer** (`src/lexer*.c`): splits the line into tokens: words and the
   operators `|`, `<`, `>`, `<<`, `>>`. Syntax errors (`| ls`, `ls >`, unclosed
   quotes, ...) are caught here.
2. **Parser** (`src/parser*.c`, `src/node*.c`): builds a tree from the
   tokens. Each command node holds its arguments and redirections; pipes are
   `PIPE` nodes. Heredocs are read now, before anything runs, into an unlinked
   temp file in `/tmp` (a pipe could only hold 64 KB).
3. **Expander** (`src/expand*.c`, `src/quotes.c`): replaces `$NAME` and `$?`
   and removes quotes, in every argument and file name.
4. **Executor** (`src/exec*.c`): walks the tree. Each pipe is created just
   before the command that writes into it, every command is forked, and the
   shell then waits for all of them. `$?` comes from the last command. A builtin
   that is not part of a pipeline runs in the shell itself, so `cd` and `export`
   change the shell.

### Source layout

| Path | What it does |
|---|---|
| `src/minishell.c` | `main()`, the prompt loop, `run_line()` |
| `src/lexer*.c` | tokenizer |
| `src/parser*.c`, `src/node*.c` | tree building and freeing |
| `src/heredoc*.c` | reading heredocs into temp files |
| `src/expand*.c`, `src/quotes.c` | `$` expansion and quote removal |
| `src/exec*.c` | redirections, PATH lookup, fork/exec, pipes, waiting |
| `src/builtins/` | the seven builtins |
| `src/envp*.c`, `src/shlvl.c`, `src/pwd.c` | the shell's own copy of the environment |
| `src/signals*.c` | signal handlers for the prompt, heredocs and children |
| `src/*err*.c` | storing and printing errors, mapping them to exit statuses |
| `include/` | one header per part, all included by `minishell.h` |
| `libft/` | our own C library |

The shell's state lives in one `t_data` struct (environment, last status, the
current line, its tokens and tree, and the current error). The only global is
`g_sig_captured`, which tells the main loop that a signal arrived.

## Resources

- [Bash Reference Manual](https://www.gnu.org/software/bash/manual/bash.html):
  the reference for every behaviour we copied
- [GNU Readline manual](https://tiswww.case.edu/php/chet/readline/readline.html)
- Linux man pages: `fork(2)`, `execve(2)`, `pipe(2)`, `dup2(2)`,
  `waitpid(2)`, `sigaction(2)`, `open(2)`, `unlink(2)`
- Real `bash`, run side by side with minishell to compare output and exit
  statuses

### Use of AI

We used Claude Code (Anthropic) as an assistant during the project, mainly to:

- discuss the design of pipes, waiting and error reporting before writing it
- review our code and point out bugs (fd leaks, wrong exit statuses)
- test minishell against bash and run valgrind and norminette checks
- make some small fixes and refactors, which we reviewed

The design decisions and the rest of the code are our own, and we can explain
every part of it.
