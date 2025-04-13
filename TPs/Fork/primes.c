#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdbool.h>

#define MIN_NUM_ARGS 2
#define NUMBER argv[1]

enum Codes { SUCCESS, ERROR_ARGS, ERROR_PIPE, ERROR_FORK };

void
send_primes_candidates(int current_pipe[2], int next_pipe[2], int p)
{
	int n;
	while (read(current_pipe[0], &n, sizeof(n)) > 0) {
		if (n % p != 0) {
			if (write(next_pipe[1], &n, sizeof(n)) < 0) {
				perror("Error al escribir en el pipe "
				       "siguiente");
				close(next_pipe[1]);
				close(current_pipe[0]);
				exit(ERROR_PIPE);
			}
		}
	}
}

void
receive_numbers(int current_pipe[2])
{
	close(current_pipe[1]);

	int p;
	if (read(current_pipe[0], &p, sizeof(p)) <= 0) {
		close(current_pipe[0]);
		exit(SUCCESS);
	}

	printf("primo %d\n", p);

	int next_pipe[2];

	if (pipe(next_pipe) < 0) {
		close(current_pipe[0]);
		exit(ERROR_PIPE);
	}

	pid_t i = fork();
	if (i < 0) {
		close(current_pipe[0]);
		close(next_pipe[0]);
		close(next_pipe[1]);
		exit(ERROR_FORK);
	}

	if (i == 0) {  // Child
		close(current_pipe[0]);
		receive_numbers(next_pipe);
	} else {  // Father
		close(next_pipe[0]);

		send_primes_candidates(current_pipe, next_pipe, p);

		close(next_pipe[1]);
		close(current_pipe[0]);
		wait(NULL);
	}
	exit(SUCCESS);
}

void
send_numbers(int current_pipe[2], int n)
{
	for (int p = 2; p <= n; p++) {
		if (write(current_pipe[1], &p, sizeof(p)) < 0) {
			perror("Error al escribir en el pipe actual");
			break;
		}
	}
}

int
main(int argc, char *argv[])
{
	if (argc < MIN_NUM_ARGS) {
		exit(ERROR_ARGS);
	}

	int current_pipe[2];

	if (pipe(current_pipe) < 0) {
		exit(ERROR_PIPE);
	}

	int n = atoi(NUMBER);

	pid_t i = fork();
	if (i < 0) {
		exit(ERROR_FORK);
	}

	if (i == 0) {  // Child
		receive_numbers(current_pipe);
	} else {  // Father
		close(current_pipe[0]);

		send_numbers(current_pipe, n);

		close(current_pipe[1]);
		wait(NULL);
	}

	exit(SUCCESS);
}
