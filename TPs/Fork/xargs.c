#ifndef NARGS
#define NARGS 4
#endif
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#define END_STRING '\0'
#define NEW_LINE '\n'
#define MIN_NUM_ARGS 2
#define COMMAND argv[1]

enum Codes { SUCCESS, ERROR_ARGS, ERROR_FORK };

void
execute_command(char *buffer[])
{
	pid_t i = fork();

	if (i < 0) {
		exit(ERROR_FORK);
	}

	if (i == 0) {  // Child
		if (execvp(buffer[0], buffer) < 0) {
			exit(ERROR_ARGS);
		}
	} else {  // Father
		wait(NULL);
	}
}

void
process_input(char *buffer[], int num_args)
{
	char *line = NULL;
	size_t size;
	int len;

	while ((len = getline(&line, &size, stdin)) != EOF) {
		if (line[len - 1] == NEW_LINE) {
			line[len - 1] = END_STRING;
		}

		buffer[num_args + 1] = line;
		num_args++;

		if (num_args ==
		    NARGS) {  // Filtro para mostrar de a NARGS argumentos
			execute_command(buffer);
			num_args = 0;
		}

		line = NULL;
	}

	if (num_args > 0) {  // Si todavia quedan argumentos por procesar
		buffer[num_args + 1] = NULL;
		execute_command(buffer);
	}
}

int
main(int argc, char *argv[])
{
	if (argc < MIN_NUM_ARGS) {
		exit(ERROR_ARGS);
	}

	char *buffer[NARGS + 2] = { NULL };  // [comando, arg1, ..., argNARGS, NULL]
	buffer[0] = COMMAND;                 // Primer argumento es el comando

	process_input(buffer, 0);

	exit(SUCCESS);
}
