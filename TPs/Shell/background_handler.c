#include "defs.h"

#include "background_handler.h"
#include "printstatus.h"

#include <errno.h>
#include <sys/wait.h>
#include <signal.h>

#include "utils.h"

char alt_stack[16384];

void
bg_handler(int)
{
	int exit_status = 0;
	int child_pid = waitpid(0, &exit_status, WNOHANG);
	if (child_pid < 0) {
#ifndef SHELL_NO_INTERACTIVE
		strerror(errno);
#endif
	} else if (child_pid > 0) {
		status = exit_status;
	}
}

void
conf_bg_handler()
{
	struct sigaction saction;
	stack_t stack;

	stack.ss_sp = alt_stack;
	stack.ss_size = sizeof(alt_stack);
	stack.ss_flags = 0;

	sigaltstack(&stack, NULL);

	memset(&saction, 0, sizeof(saction));
	saction.sa_sigaction = bg_handler;
	saction.sa_flags = SA_ONSTACK | SA_RESTART;

	// Bloquear señales durante la ejecución del manejador
	sigemptyset(&saction.sa_mask);
	sigaddset(&saction.sa_mask, SIGSEGV);
	sigaddset(&saction.sa_mask, SIGFPE);

	sigaction(SIGCHLD, &saction, NULL);
}
