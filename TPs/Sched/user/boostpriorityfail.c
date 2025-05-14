#include <inc/lib.h>

void
umain(int argc, char **argv)
{
	int p = sys_env_get_priority(0);
	cprintf("Actual priority: %005x\n", p);

	p--;

	int r = sys_env_set_priority(0, p);
	if (r >= 0)
		cprintf("Priority boost to: %005x\n", r);
	else
		cprintf("The process can't boost its own priority\n");
}