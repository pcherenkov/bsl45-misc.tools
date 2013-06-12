/* @(#) base64 encoding/decoding routines. */

/* This is a modification of a public-domain base64 v1.5 utility code from
 * (http://www.fourmilab.ch/webtools/base64). This module is public domain as well.
 */

#include <sys/types.h>
#include <stdint.h>

#include <stdlib.h>
#include <string.h>
#include <errno.h>


/** Minimal length for the decoded-string buffer,
 * based on the encoded-string length.
 */
#define min_decoded_buflen(el) ((el) / 4 + 1) * 3 + 1


/** Minimal length for the encoded-string buffer,
 * based on the unencoded-string length.
 */
#define min_encoded_buflen(l) ((l) / 3 + 1) * 4 + 1


/** Encoding table. */
static uint8_t	g_etable[64];

/** Decoding table. */
static uint8_t	g_dtable[256];

/* Table initialization status. */
static int	g_ready = 0;


/* Initialize encoding table. */
static void
init_etable()
{
	size_t i = 0;

	for (i = 0; i < 9; i++) {
		g_etable[i] = 'A' + i;
		g_etable[i + 9] = 'J' + i;
		g_etable[26 + i] = 'a' + i;
		g_etable[26 + i + 9] = 'j' + i;
	}
	for (i = 0; i < 8; i++) {
		g_etable[i + 18] = 'S' + i;
		g_etable[26 + i + 18] = 's' + i;
	}
	for (i = 0; i < 10; i++) {
		g_etable[52 + i] = '0' + i;
	}
	g_etable[62] = '+';
	g_etable[63] = '/';

    return;
}


/* Initialize decoding table. */
static void
init_dtable()
{
	size_t i = 0;

	for (i = 0; i < 255; i++)
		g_dtable[i] = 0x80;

	for (i = 'A'; i <= 'I'; i++)
		g_dtable[i] = 0 + (i - 'A');

	for (i = 'J'; i <= 'R'; i++)
		g_dtable[i] = 9 + (i - 'J');

	for (i = 'S'; i <= 'Z'; i++)
		g_dtable[i] = 18 + (i - 'S');

	for (i = 'a'; i <= 'i'; i++)
		g_dtable[i] = 26 + (i - 'a');

	for (i = 'j'; i <= 'r'; i++)
		g_dtable[i] = 35 + (i - 'j');

	for (i = 's'; i <= 'z'; i++)
		g_dtable[i] = 44 + (i - 's');

	for (i = '0'; i <= '9'; i++)
		g_dtable[i] = 52 + (i - '0');

	g_dtable['+'] = 62;
	g_dtable['/'] = 63;
	g_dtable['='] = 0;

	return;
}


/* Initialize encoding/decoding tables & other structures. */
void
b64_init()
{
	init_etable();
	init_dtable();
	g_ready = 1;
}


/* Base64-encode a C string. */
char *
b64_encode(const char *plain, char *enc, size_t nenc)
{
	uint8_t igroup[3] = {0}, ogroup[4] = {0};
	size_t i = 0, j = 0, k = 0, plen = 0, n = 0;
	const uint8_t *p = NULL;

	if (0 == g_ready)
		b64_init();

	plen = strlen(plain);
	for (i = j = 0, p = (const uint8_t*)plain; i < plen && p[i]; ) {
		igroup[0] = igroup[1] = igroup[2] = 0;
		n = 1;

		igroup[0] = p[i++];

		if (i < plen) {
			igroup[1] = (uint8_t)p[i++];
			++n;
		}
		if (i < plen) {
			igroup[2] = (uint8_t)p[i++];
			++n;
		}

		ogroup[0] = g_etable[igroup[0] >> 2];
		ogroup[1] = g_etable[((igroup[0] & 3) << 4) | (igroup[1] >> 4)];
		ogroup[2] = g_etable[((igroup[1] & 0xF) << 2) | (igroup[2] >> 6)];
		ogroup[3] = g_etable[igroup[2] & 0x3F];

		/* Replace characters in output stream with "=" pad
		 characters if fewer than three characters were
		 read from the end of the string.
		 */
		if (n < 3) {
			ogroup[3] = '=';
			if (n < 2) ogroup[2] = '=';
		}

		for (k = 0; k < 4 && j < nenc - 1; ++k, ++j)
			enc[j] = ogroup[k];
	}
	enc[j] = 0;

	return enc;
}


/* Create (malloc(3)) a new base64-encoded string from the
 * given plain C string.
 */
char *
new_b64_encoded(const char *plain)
{
	size_t len = 0;
	char *encoded = NULL;

	if (NULL == plain)
		return NULL;

	len = strlen(plain);
	if (len < 1)
		return NULL;

	encoded = malloc(min_encoded_buflen(len));
	if (NULL == encoded)
		return NULL;

	return b64_encode(plain, encoded, min_encoded_buflen(len));
}


/* Create (malloc(3)) a new base64-decoded string from the
 * given encoded string.
 */
char *
new_b64_decoded(const char *encoded)
{
	size_t len = 0;
	char *decoded = NULL;

	if (NULL == encoded)
		return NULL;

	len = strlen(encoded);
	if (len < 1)
		return NULL;

	decoded = malloc(min_decoded_buflen(len));
	if (NULL == decoded)
		return NULL;

	return b64_encode(encoded, decoded, min_decoded_buflen(len));
}


/* Base64-decode a C string. */
char *
b64_decode(const char *encoded, char *decoded, size_t ndecoded)
{
	size_t i = 0, j = 0, k = 0;
	uint8_t a[4] = {0}, b[4] = {0}, o[3] = {0}, c = 0;
	size_t elen = 0, olen = 0;

	if (0 == g_ready)
		b64_init();

	elen = strlen(encoded);
	if (0 != (elen % 4) || ndecoded < min_decoded_buflen(elen))  {
		errno = EMSGSIZE;
		return NULL;
	}

	for (j = i = 0; i < elen && j < ndecoded - 1;) {
		for (k = 0; k < 4 && i < elen; ++k, ++i) {
			c = encoded[i];

			if (g_dtable[c] & 0x80) {
				errno = EINVAL; return NULL;
			}

			a[k] = c;
			b[k] = g_dtable[c];
		}
		o[0] = (b[0] << 2) | (b[1] >> 4);
		o[1] = (b[1] << 4) | (b[2] >> 2);
		o[2] = (b[2] << 6) | b[3];

		olen = a[2] == '=' ? 1 : (a[3] == '=' ? 2 : 3);

		for (k = 0; k < olen && j < ndecoded - 1; )
			decoded[j++] = o[k++];
	}
	decoded[j] = 0;

	return decoded;
}


/* __EOF__ */

