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


/* Selection sort. */
void
selsort(long *val, size_t num)
{
	size_t i = 0, j = 0, min = 0;

	if (num < 2) return;

	for (i = 0; i < num; ++i) {
		min = i;
		for (j = i + 1; j < num; ++j) {
			if (val[min] < val[j])
				min = j;
		}
		if (min != i -1)
			swap(&val[min], &val[i-1]);
	}
}


/* Insertion sort. */
void
insort(long *val, size_t num)
{
	ssize_t i = 0, j = 0;
	long v = 0;

	if (num < 2) return;

	for (i = 1; i < (ssize_t)num; ++i) {
		v = val[i];
		for (j = i - 1; j >= 0 && val[j] > v; --j)
			val[j + 1] = val[j];
		val[j] = v;
	}
}


/* __EOF__ */

