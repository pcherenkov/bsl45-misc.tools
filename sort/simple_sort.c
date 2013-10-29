/* @(#) Simple sorting methods. */

#include <sys/types.h>
#include <stdint.h>

/* Exchange two long values. */
static inline void
swap(long *v1, long *v2)
{
	long tmp = *v1;
	*v1 = *v2;
	*v2 = tmp;
}


/* Selection sort */
void
selsort(long *val, size_t num)
{
	size_t i = 0, j = 0, min = 0;

	if (num < 2)
		return;

	for (i = 1; i < num; ++i) {
		min = i - 1;
		for (j = i; j < num; ++j) {
			if (val[min] < val[j])
				min = j;
		}
		if (min != i -1)
			swap(&val[min], &val[i-1]);
	}
}


/* __EOF__ */

