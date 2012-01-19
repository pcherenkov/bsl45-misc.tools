/* @(#) cpuid functions */

#include <sys/types.h>
#include <stdio.h>
#include <errno.h>

/* x86 flag register masks */
enum {
	cpuf_AC = (1 << 18), 	/* bit 18 */
	cpuf_ID = (1 << 21)		/* bit 21 */
};

/* CPUID-returned extended CPU features */
enum {
	xfd_HT		=	(1 << 28),
	xfd_IA64	= 	(1 << 30),
	xfc_TM2		=	(1 << 8),
	xfc_SSE_41	=	(1 << 19),
	xfc_SSE_42	=	(1 << 20),
	xfc_HYPERV	=	(1 << 31)
};


/* toggle x86 flag-register bit(s), as per mask
 */
static void
toggle_x86_flags (long mask, long* orig, long* toggled)
{
	long forig = 0, fres = 0;

#if defined (__i386__)
	asm (
		"pushfl; popl %%eax; movl %%eax, %0; xorl %2, %%eax; "
		"pushl %%eax; popfl; pushfl; popl %%eax; pushl %0; popfl "
		: "=r" (forig), "=a" (fres)
		: "m" (mask)
	);
#elif __x86_64__
	asm (
		"pushfq; popq %%rax; movq %%rax, %0; xorq %2, %%rax; "
		"pushq %%rax; popfq; pushfq; popq %%rax; pushq %0; popfq "
		: "=r" (forig), "=a" (fres)
		: "m" (mask)
	);
#else
	#error "Only x86 and x86_64 architectures supported"
#endif

	if (orig) *orig = forig;
	if (toggled) *toggled = fres;
	return;
}


static int
can_cpuid ()
{
	long of = -1, tf = -1;

	(void) printf ("Toggling AC:\n");
	toggle_x86_flags (cpuf_AC, &of, &tf);

	/* DEBUG: remove after testing */
	(void) printf ("orig_flags=[0x%lx], toggled_flags=[0x%lx]\n",
		of, tf);
	/* DEBUG END */

	if ((of & cpuf_AC) == (tf & cpuf_AC)) {
		(void) printf ("AC flags is not supported, old CPU\n");
		return 0;
	}

	(void) printf ("Toggling ID:\n");
	toggle_x86_flags (cpuf_ID, &of, &tf);

	/* DEBUG: remove after testing */
	(void) printf ("orig_flags=[0x%lx], toggled_flags=[0x%lx]\n",
		of, tf);
	/* DEBUG END */

	if ((of & cpuf_ID) == (tf & cpuf_ID)) {
		(void) printf ("CPUID flag is immutable, old CPU\n");
		return 0;
	}

	return 1;
}


static void
get_cpuid (long info, long* eax, long* ebx, long* ecx, long *edx)
{
	*eax = info;

#if defined (__i386__)
	asm __volatile__ (
		"movl %%ebx, %%edi; " 	/* PIC */
		"cpuid; "
		"movl %%ebx, %%esi; "
		"movl %%edi, %%ebx; "
		: "+a" (*eax), "=S" (*ebx), "=c" (*ecx), "=d" (*edx)
		:
		: "%edi"
	);
#elif defined (__x86_64__)
	asm __volatile__ (
		"cpuid; "
		: "+a" (*eax), "=b" (*ebx), "=c" (*ecx), "=d" (*edx)
	);
#else
	#error "Only x86 and x86_64 architectures supported"
#endif
}


static void
get_cpuid_xfeatures (long* ecx, long* edx)
{
	long info = 1, ax = 0, bx = 0;

	get_cpuid (info, &ax, &bx, ecx, edx);
	/* DEBUG */
	if (*edx & xfd_HT) 		(void) puts ("ht ");
	if (*edx & xfd_IA64) 	(void) puts ("ia64 ");
	if (*ecx & xfc_TM2) 	(void) puts ("tm2 ");
	if (*ecx & xfc_SSE_41)	(void) puts ("sse4_1 ");
	if (*ecx & xfc_SSE_42)	(void) puts ("sse4_2 ");
	if (*ecx & xfc_HYPERV)	(void) puts ("hyperv ");
	(void) fputc ('\n', stdout);
	/* DEBUG */

	return;
}



int main (int argc, char* const argv[])
{
	long c = 0, d = 0;

	(void) argc; (void) argv;
	
	if (can_cpuid ()) {
		get_cpuid_xfeatures (&c, &d);
		(void) printf ("CPU features EDX=[0x%08lx], ECX=[0x%08lx]\n", d, c);
		return 0;
	}

	return EINVAL;
}


/* __EOF__ */

