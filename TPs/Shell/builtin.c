#include "builtin.h"

#define streqword(a, b)                                                        \
	(strncmp(a, b, sizeof b - 1) == 0 &&                                   \
	 (a[sizeof b - 1] == ' ' || a[sizeof b - 1] == '\0'))

// returns true if the 'exit' call
// should be performed
//
// (It must not be called from here)
int
exit_shell(char *cmd)
{
	return streqword(cmd, "exit");
}

// returns true if "chdir" was performed
//  this means that if 'cmd' contains:
// 	1. $ cd directory (change to 'directory')
// 	2. $ cd (change to $HOME)
//  it has to be executed and then return true
//
//  Remember to update the 'prompt' with the
//  	new directory.
//
// Examples:
//  1. cmd = ['c','d', ' ', '/', 'b', 'i', 'n', '\0']
//  2. cmd = ['c','d', '\0']
int
cd(char *cmd)
{
	if (!streqword(cmd, "cd"))
		return 0;

	extern char prompt[PRMTLEN];
	char *path;
	char buf[BUFLEN] = { 0 };

	path = cmd + 2;
	while (*path == ' ')
		path++;
	if (*path == '\0') {
		path = getenv("HOME");
	}

	if (chdir(path) < 0) {
		snprintf(buf, sizeof buf, "cannot cd to %s ", path);
		perror(buf);
		status = 1;
		return 1;
	}

	if (getcwd(buf, BUFLEN) != NULL) {
		path = buf;
	}
	snprintf(prompt, PRMTLEN, "(%s)", path);
	status = 0;
	return 1;
}

// returns true if 'pwd' was invoked
// in the command line
//
// (It has to be executed here and then
// 	return true)
int
pwd(char *cmd)
{
	if (!streqword(cmd, "pwd"))
		return 0;
	char cwd[PATH_MAX];
	if (getcwd(cwd, sizeof(cwd)) != NULL) {
		printf("%s\n", cwd);
	}
	status = 0;
	return 1;
}

// returns true if `history` was invoked
// in the command line
//
// (It has to be executed here and then
// 	return true)
int
history(char *cmd)
{
	// Your code here

	return 0;
}
