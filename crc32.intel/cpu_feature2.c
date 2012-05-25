/* @(#) interface to CPU-dependent features */

#include <sys/types.h>
#include <errno.h>

#include "cpu_feature.h"

enum { eAX=0, eBX, eCX, eDX };

static const struct cpuid_feature {
	unsigned int 	ri;
	u_int32_t 		mask;
} cpu_ftr[] = {
	{eDX, (1 << 28)},	/* HT 		*/
	{eCX, (1 << 19)},	/* SSE 4.1 	*/
	{eCX, (1 << 20)},	/* SSE 4.2 	*/
	{eCX, (1 << 31)}	/* HYPERV	*/
};
static const size_t LEN_cpu_ftr = sizeof(cpu_ftr) / sizeof (cpu_ftr[0]);

#define SCALE_F		sizeof(unsigned long)

#if defined (__x86_64__)
	#define REX_PRE "0x48, "
#elif defined (__i386__)
	#define REX_PRE
#else
	#error "Only x86 and x86_64 architectures supported"
#endif


/* hw-calculate CRC32 per byte (for the unaligned portion of data buffer)
 */
static u_int32_t
crc32c_hw_byte(u_int32_t crc, unsigned char const *data, size_t length)
{
	while (length--) {
		__asm__ __volatile__(
			".byte 0xf2, 0xf, 0x38, 0xf0, 0xf1"
			:"=S"(crc)
			:"0"(crc), "c"(*data)
		);
		data++;
	}

	return crc;
}


/* hw-calculate CRC32 for the given data buffer
 */
u_int32_t
crc32c_hw(u_int32_t crc, unsigned char const *p, size_t len)
{
	unsigned int iquotient = len / SCALE_F;
	unsigned int iremainder = len % SCALE_F;
	unsigned long *ptmp = (unsigned long *)p;

	while (iquotient--) {
		__asm__ __volatile__(
			".byte 0xf2, " REX_PRE "0xf, 0x38, 0xf1, 0xf1;"
			:"=S"(crc)
			:"0"(crc), "c"(*ptmp)
		);
		ptmp++;
	}

	if (iremainder) {
		crc = crc32c_hw_byte(crc, (unsigned char *)ptmp,
				 			iremainder);
	}

	return crc;
}


/* toggle x86 flag-register bits, as per mask
   return flags both in original and toggled state
 */
static void
toggle_x86_flags (long mask, long* orig, long* toggled)
{
#if defined (__i386__)
	asm __volatile__ (
		"pushfl; popl %%eax; movl %%eax, %0; xorl %2, %%eax; "
		"pushl %%eax; popfl; pushfl; popl %%eax; pushl %0; popfl "
		: "=b" (*orig), "=a" (*toggled)
		: "D" (mask)
	);
#elif __x86_64__
	asm __volatile__ (
		"pushfq; popq %%rax; movq %%rax, %0; xorq %2, %%rax; "
		"pushq %%rax; popfq; pushfq; popq %%rax; pushq %0; popfq "
		: "=b" (*orig), "=a" (*toggled)
		: "D" (mask)
	);
#endif
}


/* is CPUID instruction available ? */
static int
can_cpuid (int *err)
{
	long orig = -1, toggled = -1;

	/* x86 flag register masks */
	enum {
		cpuf_AC = (1 << 18), 	/* bit 18 */
		cpuf_ID = (1 << 21)	/* bit 21 */
	};

	enum { ERR_i386 = -10, ERR_NO_CPUID = -11 };

	/* check if AC (alignment) flag could be toggled:
		if not - it's i386, thus no CPUID
	*/
	toggle_x86_flags (cpuf_AC, &orig, &toggled);
	if ((orig & cpuf_AC) == (toggled & cpuf_AC)) {
		*err = ERR_i386;
		return 0;
	}

	/* next try toggling CPUID (ID) flag */
	toggle_x86_flags (cpuf_ID, &orig, &toggled);
	if ((orig & cpuf_ID) == (toggled & cpuf_ID)) {
		*err = ERR_NO_CPUID;
		return 0;
	}

	return 1;
}


/* retrieve CPUID data using info as the EAX key */
static void
get_cpuid (long info, long* eax, long* ebx, long* ecx, long *edx)
{
	*eax = info;

#if defined (__i386__)
	asm __volatile__ (
		"movl %%ebx, %%edi; " 	/* must save ebx for 32-bit PIC code */
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
#endif
}


/* return 1=feature is available, 0=unavailable, 0>(errno) = error */
int
cpu_has (unsigned int feature)
{
	long info = 1, reg[4] = {0,0,0,0};
	int err = 0;

	/* TODO: in AMD64 cpuid is always there */
	if (!can_cpuid(&err))
		return err;

	if (feature > LEN_cpu_ftr)
		return -ERANGE;

	get_cpuid (info, &reg[eAX], &reg[eBX], &reg[eCX], &reg[eDX]);

	return (reg[cpu_ftr[feature].ri] & cpu_ftr[feature].mask) ? 1 : 0;
}


/* __EOF__ */

