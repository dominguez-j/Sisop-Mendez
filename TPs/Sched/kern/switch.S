/*
 * Your code here for context_switch
 *
 * It should restore the register values in the Trapframe with the 'iret' instruction.
 * This exits the kernel and starts executing some environment's code.
 *
 * This function does not return.
 */

.globl context_switch;
context_switch:

/*
 * Se llama para volver a user-space, restaura los registros guardados en el Trapframe
 * No hace return, cambia el contexto
 * inverso de _alltraps en trapentry.s
 */

	movl 4(%esp), %esp
	popal
	popl %es
	popl %ds
	addl $8, %esp
	iret

spin:
	jmp spin
