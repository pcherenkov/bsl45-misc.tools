/* @(#) interface to CPU-dependent features */
/* NB: must use -msse3 -msse4 gcc options to compile */

#if !defined (__x86_64__) && !defined (__i386__)
	#error "Only x86 and x86_64 architectures supported"
#endif

#ifndef __GNUC__
#error This module uses GCC intrinsic header(s) and should be compiled using gcc.
#endif

#include <sys/types.h>
#include <errno.h>

/* GCC intrinsic headers */
#include <cpuid.h>
#include <smmintrin.h>

#include "cpu_fi.h"


/* hw-calculate CRC32 for the given data buffer
 */
u_int32_t
crc32c_hw(u_int32_t crc, unsigned char const *p, size_t len)
{
#define SCALE_F	sizeof(unsigned long)
	size_t nwords = len / SCALE_F, nbytes = len % SCALE_F;
	unsigned long *pword = (unsigned long *)p;
	unsigned char *pbyte = NULL;


	for (; nwords--; ++pword) {
#if defined (__x86_64__)
		crc = (u_int32_t)_mm_crc32_u64((u_int64_t)crc, *pword);
#elif defined (__i386__)
		crc = _mm_crc32_u32(crc, *pword);
#endif
	}

	if (nbytes)
		for (pbyte = (unsigned char*)pword; nbytes--; ++pbyte)
			crc = _mm_crc32_u8(crc, *pbyte);

	return crc;
}


int
sse42_enabled_cpu()
{
	unsigned int ax, bx, cx, dx;

	if (__get_cpuid(1 /* level */, &ax, &bx, &cx, &dx) == 0)
		return 0; /* not supported */

	return (cx & (1 << 20)) ? 1 : 0;
}


/* __EOF__ */

