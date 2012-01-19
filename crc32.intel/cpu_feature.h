#ifndef TARANTOOL_CPU_FEATURES_H
#define TARANTOOL_CPU_FEATURES_H

#include <sys/types.h>

/* CPU feature capabilities to use with cpu_has (feature) */
enum {
	cpuf_ht = 0, cpuf_sse4_1, cpuf_sse4_2, cpuf_hypervisor
};

/*	return 1=feature is available, 0=unavailable, -EINVAL = unsupported CPU,
	-ERANGE = invalid feature
*/
int cpu_has (unsigned int feature);


/* hardware-calculate CRC32 for the given data buffer
 * NB: 	requires 1 == cpu_has (cpuf_sse4_2),
 * 		CALLING IT W/O CHECKING for sse4_2 CAN CAUSE SIGABRT
 */
u_int32_t crc32c_hw(u_int32_t crc, unsigned char const *p, size_t len);


#endif /* TARANTOOL_CPU_FEATURES_H */

/* __EOF__ */

