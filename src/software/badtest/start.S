/* I truly do hate AT&T assembly... */

	.text
	.p2align 2
	.globl	_start
_start:
	/*popq	%rdi*/
	mov %rsp, %rdi
	call	_start_c
	.size	_start,.-_start