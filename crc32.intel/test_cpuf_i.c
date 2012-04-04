/* @(#) unit test for cpu_feature.c */

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>

#include "crc32.h"
#include "cpu_fi.h"

int main (int argc, char* const argv[])
{
	int rc = 0, seed = -1;
	long len = -1, i = -1;
	u_int32_t crc = 0;
	unsigned char *buf = NULL;
	static const long MAX_LEN = 1024*1024*1000*2;
	clock_t clk[2] = {0,0}, clk_hw = 0, clk_sw = 0;

	if (argc < 3) {
		(void) fprintf (stderr, "Usage: %s seed length\n", argv[0]);
		return 1;
	}

	seed = atoi (argv[1]);
	if (seed <= 0) {
		(void) fprintf (stderr, "%s: invalid seed value: %s\n",
			argv[0], argv[1]);
		return EINVAL;
	}

	len = atol (argv[2]);
	if ((len <= 0) || (len > MAX_LEN)) {
		(void) fprintf (stderr, "%s: invalid length value: %s\n",
			argv[0], argv[2]);
		return EINVAL;
	}

	if (0 == sse42_enabled_cpu()) {
		(void) fprintf (stderr, "%s: sorry, this CPU does not support SSE 4.2 "
			"and cannot calculate CRC32 in hardware\n", argv[0]);
		return EINVAL;
	}

	srandom (seed);
	buf = malloc ((size_t)len);
	if (!buf) {
		(void) fprintf (stderr, "%s: out of memory\n", argv[0]);
		return ENOMEM;
	}
	(void) printf ("Generating buffer len=[%ld], seed=[%d] : ", len, seed);
	for (i = 0; i < len; ++i) buf[i] = (unsigned char)(random () & 0xff);
	(void) fputs ("done\n", stdout);

	clk[0] = clock ();
	for (i = 0; i < 1000; ++i)
		crc = crc32c_hw (0, buf, (size_t)len);
	clk[1] = clock ();

	clk_hw = clk[1]-clk[0];
	(void) printf ("HW CRC32 = [%lx], time=[%ld] clks\n", (long)crc, (long)clk_hw);

	clk[0] = clock ();
	for (i = 0; i < 1000; ++i)
		crc = crc32c (0, buf, (size_t)len);
	clk[1] = clock ();

	clk_sw = clk[1]-clk[0];
	(void) printf ("SW CRC32 = [%lx], time=[%ld] clks\n", (long)crc, (long)clk_sw);

	(void) printf ("HW/SW = %.6f, SW/HW=%.6f\n", (double)clk_hw/(double)clk_sw,
		(double)clk_sw/(double)clk_hw);

	free (buf);
	return rc;
}


/* __EOF__ */

