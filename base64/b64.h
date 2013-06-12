/* @(#) C89-compliant base64 encoding/decoding routines. */

#ifndef B64C89_H_20130611
#define B64C89_H_20130611

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize internal context (to avoid
 * delays at on-the-spot initialization).
 */
void b64_init();


/**
 * Base64-encode a C string.
 *
 * @param plain		C string to encode.
 * @param enc		destination buffer.
 * @param elen		buffer size.
 *
 * @return encoded string in enc or NULL if error.
 */
char* b64_encode(const char *plain, char *enc, size_t elen);

/**
 * Base64-decode a C string.
 *
 * @param enc		encoded C string to decode.
 * @param decoded	destination buffer.
 * @param dlen		buffer size.
 *
 * @return decoded string in decoded or NULL if error.
 */
char *
b64_decode(const char *enc, char *decoded, size_t dlen);


/**
 * Create (malloc(3)) a new base64-encoded string from the
 * given un-encoded (plain) C string.
 *
 * @param plain		C string to decode.
 * @return		fresh-allocated encoded string (to free(3) later).
 */
char *
new_b64_encoded(const char *plain);


/**
 * Create (malloc(3)) a new base64-decoded string from the
 * given encoded string.
 *
 * @param encoded	base64-encoded string to decode.
 * @return		fresh-allocated decoded string (to free(3) later).
 */
char *
new_b64_decoded(const char *encoded);


#ifdef __cplusplus
}
#endif

#endif /* B64C89_H_20130611*/

/* __EOF__ */

