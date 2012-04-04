#ifndef CPU_FEATURES_H
#define CPU_FEATURES_H

#include <sys/types.h>

/*	return 1=feature is available, 0=unavailable
*/
int sse42_enabled_cpu();


/* hardware-calculate CRC32 for the given data buffer
 * NB: 	requires 1 == sse42_enabled_cpu(),
 * 		CALLING IT W/O CHECKING for sse4_2 CAN CAUSE SIGILL
 */
u_int32_t crc32c_hw(u_int32_t crc, unsigned char const *p, size_t len);


#endif /* TARANTOOL_CPU_FEATURES_H */

/* __EOF__ */

