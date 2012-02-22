	.file	"unalign.c"
	.section	.rodata.str1.1,"aMS",@progbits,1
.LC0:
	.string	"Usage: %s size offset\n"
.LC1:
	.string	"Invalid size value: %s"
.LC2:
	.string	"Invalid offset value: %s"
.LC3:
	.string	"mmap"
.LC4:
	.string	"BEGIN vp=%p\n"
.LC5:
	.string	"END vp=%p\n"
.LC6:
	.string	"munmap"
	.section	.text.startup,"ax",@progbits
	.p2align 4,,15
	.globl	main
	.type	main, @function
main:
.LFB34:
	.cfi_startproc
	pushq	%r12
	.cfi_def_cfa_offset 16
	.cfi_offset 12, -16
	movq	%rsi, %r12
	pushq	%rbp
	.cfi_def_cfa_offset 24
	.cfi_offset 6, -24
	pushq	%rbx
	.cfi_def_cfa_offset 32
	.cfi_offset 3, -32
	subq	$16, %rsp
	.cfi_def_cfa_offset 48
	cmpl	$2, %edi
	jle	.L12
	movq	8(%rsi), %rdi
	movl	$10, %edx
	xorl	%esi, %esi
	call	strtol
	testq	%rax, %rax
	movq	%rax, %rbx
	jle	.L13
	movq	16(%r12), %rdi
	xorl	%esi, %esi
	movl	$10, %edx
	call	strtol
	testq	%rax, %rax
	movq	%rax, %rbp
	jle	.L14
	xorl	%r9d, %r9d
	orl	$-1, %r8d
	xorl	%edi, %edi
	movl	$34, %ecx
	movl	$3, %edx
	movq	%rbx, %rsi
	call	mmap
	cmpq	$-1, %rax
	movq	%rax, %r12
	je	.L15
	movq	stdout(%rip), %rdi
	addq	%rax, %rbp
	movl	$.LC4, %edx
	xorl	%eax, %eax
	movq	%rbp, %rcx
	movl	$1, %esi
	call	__fprintf_chk
	leaq	(%r12,%rbx), %rdx
	cmpq	%rdx, %rbp
	jae	.L7
	movq	%rbp, %rax
	.p2align 4,,10
	.p2align 3
.L8:
	movq	$0, (%rax)
	addq	$8, %rax
	cmpq	%rdx, %rax
	jb	.L8
	movq	%rbp, %rax
	notq	%rax
	addq	%rdx, %rax
	shrq	$3, %rax
	leaq	8(%rbp,%rax,8), %rbp
.L7:
	movq	stdout(%rip), %rdi
	movq	%rbp, %rcx
	movl	$.LC5, %edx
	movl	$1, %esi
	xorl	%eax, %eax
	call	__fprintf_chk
	movq	%rbx, %rsi
	movq	%r12, %rdi
	call	munmap
	cmpl	$-1, %eax
	je	.L16
.L3:
	addq	$16, %rsp
	.cfi_remember_state
	.cfi_def_cfa_offset 32
	popq	%rbx
	.cfi_def_cfa_offset 24
	popq	%rbp
	.cfi_def_cfa_offset 16
	popq	%r12
	.cfi_def_cfa_offset 8
	ret
.L12:
	.cfi_restore_state
	movq	(%rsi), %rcx
	movl	$.LC0, %edx
.L10:
	movq	stderr(%rip), %rdi
	movl	$1, %esi
	xorl	%eax, %eax
	call	__fprintf_chk
	movl	$1, %eax
	jmp	.L3
.L13:
	movq	8(%r12), %rcx
	movl	$.LC1, %edx
	jmp	.L10
.L14:
	movq	8(%r12), %rcx
	movl	$.LC2, %edx
	jmp	.L10
.L16:
	movl	$.LC6, %edi
	movl	%eax, 8(%rsp)
	call	perror
	movl	8(%rsp), %eax
	jmp	.L3
.L15:
	movl	$.LC3, %edi
	call	perror
	movl	$1, %eax
	jmp	.L3
	.cfi_endproc
.LFE34:
	.size	main, .-main
	.ident	"GCC: (Ubuntu/Linaro 4.6.1-9ubuntu3) 4.6.1"
	.section	.note.GNU-stack,"",@progbits
