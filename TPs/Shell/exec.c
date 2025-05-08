#include "defs.h"

#include "exec.h"

// sets "key" with the key part of "arg"
// and null-terminates it
//
// Example:
//  - KEY=value
//  arg = ['K', 'E', 'Y', '=', 'v', 'a', 'l', 'u', 'e', '\0']
//  key = "KEY"
//
// static void
// get_environ_key(char *arg, char *key)
// {
// 	int i;
// 	for (i = 0; arg[i] != '='; i++)
// 		key[i] = arg[i];

// 	key[i] = END_STRING;
// }

// sets "value" with the value part of "arg"
// and null-terminates it
// "idx" should be the index in "arg" where "=" char
// resides
//
// Example:
//  - KEY=value
//  arg = ['K', 'E', 'Y', '=', 'v', 'a', 'l', 'u', 'e', '\0']
//  value = "value"
//
// static void
// get_environ_value(char *arg, char *value, int idx)
// {
// 	size_t i, j;
// 	for (i = (idx + 1), j = 0; i < strlen(arg); i++, j++)
// 		value[j] = arg[i];

// 	value[j] = END_STRING;
// }

// sets the environment variables received
// in the command line
//
// Hints:
// - use 'block_contains()' to
// 	get the index where the '=' is
// - 'get_environ_*()' can be useful here
static void
set_environ_vars(char **eargv, int eargc)
{
	for (int i = 0; i < eargc; i++) {
		if (putenv(eargv[i]) != 0) {
			perror("couldn't set environment variable");
		}
	}
}

// opens the file in which the stdin/stdout/stderr
// flow will be redirected, and returns
// the file descriptor
//
// Find out what permissions it needs.
// Does it have to be closed after the execve(2) call?
//
// Hints:
// - if O_CREAT is used, add S_IWUSR and S_IRUSR
// 	to make it a readable normal file

// fd indicates the fd to be redirected
static int
open_redir_fd(char *file, int flags, int fd)
{
	int fd_open = open(file, flags | O_CLOEXEC, S_IWUSR | S_IRUSR);
	return dup2(fd_open, fd);
}

// executes a command - does not return
//
// Hint:
// - check how the 'cmd' structs are defined
// 	in types.h
// - casting could be a good option
void
exec_cmd(struct cmd *cmd)
{
	// To be used in the different cases
	struct execcmd *e;
	struct backcmd *b;
	struct execcmd *r;
	struct pipecmd *p;

	switch (cmd->type) {
	case EXEC:
		// spawns a command

		e = (struct execcmd *) cmd;
		set_environ_vars(e->eargv, e->eargc);
		execvp(e->argv[0], e->argv);
		perror("execvp falló");
		_exit(-1);
		break;

	case BACK:
		b = (struct backcmd *) cmd;
		exec_cmd(b->c);
		break;

	case REDIR:
		// changes the input/output/stderr flow
		//
		// To check if a redirection has to be performed
		// verify if file name's length (in the execcmd struct)
		// is greater than zero

		r = (struct execcmd *) cmd;
		// checks if any redirection has to be performed

		// stdin redir
		if (strlen(r->in_file) > 0) {
			if (open_redir_fd(r->in_file, O_RDONLY, STDIN_FILENO) < 0) {
				perror("stdin redirection failed");
				_exit(-1);
			}
		}
		// stdout redir
		if (strlen(r->out_file) > 0) {
			if (open_redir_fd(r->out_file,
			                  O_WRONLY | O_CREAT | O_TRUNC,
			                  STDOUT_FILENO) < 0) {
				perror("stdout redirection failed");
				_exit(-1);
			}
		}
		// stderr redir
		if (strlen(r->err_file) > 0) {
			if (strcmp(r->err_file, "&1") == 0) {
				if (dup2(1, 2) < 0) {
					perror("stderr redirection failed");
					_exit(-1);
				}
			} else {
				if (open_redir_fd(r->err_file,
				                  O_WRONLY | O_CREAT | O_TRUNC,
				                  STDERR_FILENO) < 0) {
					perror("stderr redirection failed");
					_exit(-1);
				}
			}
		}
		r->type = EXEC;
		exec_cmd(cmd);
		break;

	case PIPE:
		// pipes two commands

		p = (struct pipecmd *) cmd;
		int fd[2];
		if (pipe(fd) < 0) {
			perror("pipe failed");
			_exit(-1);
		}
		pid_t p1, p2;
		if ((p1 = fork()) == 0) {
			// Hijo 1
			dup2(fd[WRITE], STDOUT_FILENO);
			close(fd[READ]);
			close(fd[WRITE]);
			exec_cmd(p->leftcmd);
		} else if ((p2 = fork()) == 0) {
			// Hijo 2
			dup2(fd[READ], STDIN_FILENO);
			close(fd[READ]);
			close(fd[WRITE]);
			exec_cmd(p->rightcmd);
		}

		// close the file descriptors
		close(fd[READ]);
		close(fd[WRITE]);
		waitpid(p1, NULL, 0);
		waitpid(p2, &status, 0);

		// free the memory allocated
		// for the pipe tree structure
		free_command(parsed_pipe);
		break;
	}
	exit(0);
}
