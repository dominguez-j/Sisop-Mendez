#include <inc/assert.h>
#include <inc/x86.h>
#include <kern/spinlock.h>
#include <kern/env.h>
#include <kern/pmap.h>
#include <kern/monitor.h>
#include <kern/sched_stats.h>

#define MIN_PRIORITY 0x100
struct SchedStats stats = { 0, 0, 0 };

void sched_halt(void);

// Choose a user environment to run and run it.
void
sched_yield(void)
{
	stats.number_of_yields++;
	struct Env *next = NULL;
#ifdef SCHED_ROUND_ROBIN
	// Implement simple round-robin scheduling.
	//
	// Search through 'envs' for an ENV_RUNNABLE environment in
	// circular fashion starting just after the env this CPU was
	// last running. Switch to the first such environment found.
	//
	// If no envs are runnable, but the environment previously
	// running on this CPU is still ENV_RUNNING, it's okay to
	// choose that environment.
	//
	// Never choose an environment that's currently running on
	// another CPU (env_status == ENV_RUNNING). If there are
	// no runnable environments, simply drop through to the code
	// below to halt the cpu.

	// Your code here - Round robin
	int i = curenv ? ENVX(curenv->env_id) + 1 : 0;
	int start = i;

	do {
		if (envs[i].env_status == ENV_RUNNABLE)
			next = &envs[i];

		i = (i + 1) % NENV;
	} while (i != start && !next);

#endif

#ifdef SCHED_PRIORITIES
	// Implement simple priorities scheduling.
	//
	// Environments now have a "priority" so it must be consider
	// when the selection is performed.
	//
	// Be careful to not fall in "starvation" such that only one
	// environment is selected and run every time.

	// Your code here - Priorities

	// Every 10 switches, boost all runnable environments
	if (stats.number_of_switches % 10 == 0) {
		stats.number_of_boosts++;
		for (int i = 0; i < NENV; i++) {
			if (envs[i].env_status == ENV_RUNNABLE)
				envs[i].env_priority = 0;
		}
	}

	int min_priority = MIN_PRIORITY;

	int i = curenv ? ENVX(curenv->env_id) + 1 : 0;
	int start = i;

	do {
		if (envs[i].env_status == ENV_RUNNABLE &&
		    envs[i].env_priority < min_priority) {
			min_priority = envs[i].env_priority;
			next = &envs[i];
		}

		i = (i + 1) % NENV;
	} while (i != start && !next);

	// Increment priority of current environment if it is not the next one
	if (curenv && next) {
		if (curenv->env_priority < MIN_PRIORITY - 1)
			curenv->env_priority++;
	}

#endif

	// Without scheduler, keep runing the last environment while it exists
	if (next) {
		stats.number_of_switches++;
		env_run(next);
	} else if (curenv && curenv->env_status == ENV_RUNNING &&
	           curenv->env_cpunum == cpunum()) {
		env_run(curenv);
	}

	// sched_halt never returns
	sched_halt();
}

void
print_sched_stats()
{
	for (int i = 0; i < NENV; i++) {
		if (envs[i].env_runs == 0)
			continue;
		cprintf("Process ID: %005x, number of runs: %005x \n",
		        envs[i].env_id,
		        envs[i].env_runs);
	}


	cprintf("Number of context switch: %005x\n", stats.number_of_switches);
	cprintf("Number of yields: %005x\n", stats.number_of_yields);
	cprintf("Number of boosts: %005x\n", stats.number_of_boosts);
}

// Halt this CPU when there is nothing to do. Wait until the
// timer interrupt wakes it up. This function never returns.
//
void
sched_halt(void)
{
	int i;

	// For debugging and testing purposes, if there are no runnable
	// environments in the system, then drop into the kernel monitor.
	for (i = 0; i < NENV; i++) {
		if ((envs[i].env_status == ENV_RUNNABLE ||
		     envs[i].env_status == ENV_RUNNING ||
		     envs[i].env_status == ENV_DYING))
			break;
	}
	if (i == NENV) {
		cprintf("No runnable environments in the system!\n");
		print_sched_stats();
		while (1)
			monitor(NULL);
	}

	// Mark that no environment is running on this CPU
	curenv = NULL;
	lcr3(PADDR(kern_pgdir));

	// Mark that this CPU is in the HALT state, so that when
	// timer interupts come in, we know we should re-acquire the
	// big kernel lock
	xchg(&thiscpu->cpu_status, CPU_HALTED);

	// Release the big kernel lock as if we were "leaving" the kernel
	unlock_kernel();

	// Once the scheduler has finishied it's work, print statistics on
	// performance. Your code here

	// Reset stack pointer, enable interrupts and then halt.
	asm volatile("movl $0, %%ebp\n"
	             "movl %0, %%esp\n"
	             "pushl $0\n"
	             "pushl $0\n"
	             "sti\n"
	             "1:\n"
	             "hlt\n"
	             "jmp 1b\n"
	             :
	             : "a"(thiscpu->cpu_ts.ts_esp0));
}
