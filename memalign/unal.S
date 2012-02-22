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
	jle	.L20
	movq	8(%rsi), %rdi
	movl	$10, %edx
	xorl	%esi, %esi
	call	strtol
	testq	%rax, %rax
	movq	%rax, %rbx
	jle	.L21
	movq	16(%r12), %rdi
	xorl	%esi, %esi
	movl	$10, %edx
	call	strtol
	testq	%rax, %rax
	movq	%rax, %rbp
	jle	.L22
	xorl	%r9d, %r9d
	orl	$-1, %r8d
	xorl	%edi, %edi
	movl	$34, %ecx
	movl	$3, %edx
	movq	%rbx, %rsi
	call	mmap
	cmpq	$-1, %rax
	movq	%rax, %r12
	je	.L23
	movq	stdout(%rip), %rdi
	addq	%rax, %rbp
	movl	$1, %esi
	xorl	%eax, %eax
	movq	%rbp, %rcx
	movl	$.LC4, %edx
	call	__fprintf_chk
	leaq	(%r12,%rbx), %rsi
	cmpq	%rsi, %rbp
	jae	.L7
	leaq	8(%rbp), %rax
	leaq	7(%rsi), %r8
	movq	%rbp, %rdx
	salq	$60, %rdx
	subq	%rax, %r8
	shrq	$63, %rdx
	shrq	$3, %r8
	addq	$1, %r8
	cmpq	%rdx, %r8
	cmovbe	%r8, %rdx
	testq	%rdx, %rdx
	je	.L14
	cmpq	%rdx, %r8
	movq	$0, 0(%rbp)
	je	.L9
.L8:
	movq	%r8, %r10
	subq	%rdx, %r10
	movq	%r10, %rdi
	shrq	%rdi
	movq	%rdi, %r9
	addq	%r9, %r9
	je	.L15
	pxor	%xmm0, %xmm0
	leaq	0(%rbp,%rdx,8), %rcx
	xorl	%edx, %edx
	.p2align 4,,10
	.p2align 3
.L11:
	addq	$1, %rdx
	movdqa	%xmm0, (%rcx)
	addq	$16, %rcx
	cmpq	%rdi, %rdx
	jb	.L11
	cmpq	%r9, %r10
	leaq	(%rax,%r9,8), %rax
	je	.L9
	.p2align 4,,10
	.p2align 3
.L15:
	movq	$0, (%rax)
	addq	$8, %rax
	cmpq	%rax, %rsi
	ja	.L15
.L9:
	leaq	0(%rbp,%r8,8), %rbp
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
	je	.L24
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
.L20:
	.cfi_restore_state
	movq	(%rsi), %rcx
	movl	$.LC0, %edx
.L18:
	movq	stderr(%rip), %rdi
	movl	$1, %esi
	xorl	%eax, %eax
	call	__fprintf_chk
	movl	$1, %eax
	jmp	.L3
.L21:
	movq	8(%r12), %rcx
	movl	$.LC1, %edx
	jmp	.L18
.L22:
	movq	8(%r12), %rcx
	movl	$.LC2, %edx
	jmp	.L18
.L24:
	movl	$.LC6, %edi
	movl	%eax, 8(%rsp)
	call	perror
	movl	8(%rsp), %eax
	jmp	.L3
.L23:
	movl	$.LC3, %edi
	call	perror
	movl	$1, %eax
	jmp	.L3
.L14:
	movq	%rbp, %rax
	jmp	.L8
	.cfi_endproc
.LFE34:
	.size	main, .-main
	.ident	"GCC: (Ubuntu/Linaro 4.6.1-9ubuntu3) 4.6.1"
	.section	.note.GNU-stack,"",@progbits
