/**
  * Thanks to John Walker (http://www.fourmilab.ch/md5/) for the great MD5 implementation
 **/

#ifndef MD5_H
#define MD5_H

#include <memory.h>		 /* for memcpy() */
#include <stdint.h>

/*  The following tests optimise behaviour on little-endian
    machines, where there is no need to reverse the byte order
    of 32 bit words in the MD5 computation.  By default,
    MD5_HIGHFIRST is defined, which indicates we're running on a
    big-endian (most significant byte first) machine, on which
    the MD5_byteReverse function in md5.c must be invoked. However,
    MD5_byteReverse is coded in such a way that it is an identity
    function when run on a little-endian machine, so calling it
    on such a platform causes no harm apart from wasting time. 
    If the platform is known to be little-endian, we speed
    things up by undefining MD5_HIGHFIRST, which defines
    MD5_byteReverse as a null macro.  Doing things in this manner
    insures we work on new platforms regardless of their byte
    order.  */

#define MD5_HIGHFIRST

#if defined(__i386__) || defined(__x86_64__) || defined(__amd64__)
#undef MD5_HIGHFIRST
#endif

/*  On machines where "long" is 64 bits, we need to declare
    uint32 as something guaranteed to be 32 bits.  */

typedef uint32_t uint32;

struct MD5Context {
        uint32 buf[4];
        uint32 bits[2];
        unsigned char in[64];
};

/*
 * This is needed to make RSAREF happy on some MS-DOS compilers.
 */
typedef struct MD5Context MD5_CTX;

#ifndef MD5_HIGHFIRST
#define MD5_byteReverse(buf, len)	/* Nothing */
#else
/*
 * Note: this code is harmless on little-endian machines.
 */
inline static void MD5_byteReverse(unsigned char *buf, unsigned longs)
{
    uint32 t;
    do {
	t = (uint32) ((unsigned) buf[3] << 8 | buf[2]) << 16 |
	    ((unsigned) buf[1] << 8 | buf[0]);
	*(uint32 *) buf = t;
	buf += 4;
    } while (--longs);
}
#endif

/*
 * Start MD5 accumulation.  Set bit count to 0 and buffer to mysterious
 * initialization constants.
 */
inline static void MD5Init(struct MD5Context *ctx)
{
    ctx->buf[0] = 0x67452301;
    ctx->buf[1] = 0xefcdab89;
    ctx->buf[2] = 0x98badcfe;
    ctx->buf[3] = 0x10325476;

    ctx->bits[0] = 0;
    ctx->bits[1] = 0;
}


/* The four core functions - MD5_F1 is optimized somewhat */

/* #define MD5_F1(x, y, z) (x & y | ~x & z) */
#define MD5_F1(x, y, z) (z ^ (x & (y ^ z)))
#define MD5_F2(x, y, z) MD5_F1(z, x, y)
#define MD5_F3(x, y, z) (x ^ y ^ z)
#define MD5_F4(x, y, z) (y ^ (x | ~z))

/* This is the central step in the MD5 algorithm. */
#define MD5STEP(f, w, x, y, z, data, s) \
	( w += f(x, y, z) + data,  w = w<<s | w>>(32-s),  w += x )

/*
 * The core of the MD5 algorithm, this alters an existing MD5 hash to
 * reflect the addition of 16 longwords of new data.  MD5Update blocks
 * the data and converts bytes into longwords for this routine.
 */
inline static void MD5Transform(uint32 buf[4], uint32 in[16])
{
    register uint32 a, b, c, d;

    a = buf[0];
    b = buf[1];
    c = buf[2];
    d = buf[3];

    MD5STEP(MD5_F1, a, b, c, d, in[0] + 0xd76aa478, 7);
    MD5STEP(MD5_F1, d, a, b, c, in[1] + 0xe8c7b756, 12);
    MD5STEP(MD5_F1, c, d, a, b, in[2] + 0x242070db, 17);
    MD5STEP(MD5_F1, b, c, d, a, in[3] + 0xc1bdceee, 22);
    MD5STEP(MD5_F1, a, b, c, d, in[4] + 0xf57c0faf, 7);
    MD5STEP(MD5_F1, d, a, b, c, in[5] + 0x4787c62a, 12);
    MD5STEP(MD5_F1, c, d, a, b, in[6] + 0xa8304613, 17);
    MD5STEP(MD5_F1, b, c, d, a, in[7] + 0xfd469501, 22);
    MD5STEP(MD5_F1, a, b, c, d, in[8] + 0x698098d8, 7);
    MD5STEP(MD5_F1, d, a, b, c, in[9] + 0x8b44f7af, 12);
    MD5STEP(MD5_F1, c, d, a, b, in[10] + 0xffff5bb1, 17);
    MD5STEP(MD5_F1, b, c, d, a, in[11] + 0x895cd7be, 22);
    MD5STEP(MD5_F1, a, b, c, d, in[12] + 0x6b901122, 7);
    MD5STEP(MD5_F1, d, a, b, c, in[13] + 0xfd987193, 12);
    MD5STEP(MD5_F1, c, d, a, b, in[14] + 0xa679438e, 17);
    MD5STEP(MD5_F1, b, c, d, a, in[15] + 0x49b40821, 22);

    MD5STEP(MD5_F2, a, b, c, d, in[1] + 0xf61e2562, 5);
    MD5STEP(MD5_F2, d, a, b, c, in[6] + 0xc040b340, 9);
    MD5STEP(MD5_F2, c, d, a, b, in[11] + 0x265e5a51, 14);
    MD5STEP(MD5_F2, b, c, d, a, in[0] + 0xe9b6c7aa, 20);
    MD5STEP(MD5_F2, a, b, c, d, in[5] + 0xd62f105d, 5);
    MD5STEP(MD5_F2, d, a, b, c, in[10] + 0x02441453, 9);
    MD5STEP(MD5_F2, c, d, a, b, in[15] + 0xd8a1e681, 14);
    MD5STEP(MD5_F2, b, c, d, a, in[4] + 0xe7d3fbc8, 20);
    MD5STEP(MD5_F2, a, b, c, d, in[9] + 0x21e1cde6, 5);
    MD5STEP(MD5_F2, d, a, b, c, in[14] + 0xc33707d6, 9);
    MD5STEP(MD5_F2, c, d, a, b, in[3] + 0xf4d50d87, 14);
    MD5STEP(MD5_F2, b, c, d, a, in[8] + 0x455a14ed, 20);
    MD5STEP(MD5_F2, a, b, c, d, in[13] + 0xa9e3e905, 5);
    MD5STEP(MD5_F2, d, a, b, c, in[2] + 0xfcefa3f8, 9);
    MD5STEP(MD5_F2, c, d, a, b, in[7] + 0x676f02d9, 14);
    MD5STEP(MD5_F2, b, c, d, a, in[12] + 0x8d2a4c8a, 20);

    MD5STEP(MD5_F3, a, b, c, d, in[5] + 0xfffa3942, 4);
    MD5STEP(MD5_F3, d, a, b, c, in[8] + 0x8771f681, 11);
    MD5STEP(MD5_F3, c, d, a, b, in[11] + 0x6d9d6122, 16);
    MD5STEP(MD5_F3, b, c, d, a, in[14] + 0xfde5380c, 23);
    MD5STEP(MD5_F3, a, b, c, d, in[1] + 0xa4beea44, 4);
    MD5STEP(MD5_F3, d, a, b, c, in[4] + 0x4bdecfa9, 11);
    MD5STEP(MD5_F3, c, d, a, b, in[7] + 0xf6bb4b60, 16);
    MD5STEP(MD5_F3, b, c, d, a, in[10] + 0xbebfbc70, 23);
    MD5STEP(MD5_F3, a, b, c, d, in[13] + 0x289b7ec6, 4);
    MD5STEP(MD5_F3, d, a, b, c, in[0] + 0xeaa127fa, 11);
    MD5STEP(MD5_F3, c, d, a, b, in[3] + 0xd4ef3085, 16);
    MD5STEP(MD5_F3, b, c, d, a, in[6] + 0x04881d05, 23);
    MD5STEP(MD5_F3, a, b, c, d, in[9] + 0xd9d4d039, 4);
    MD5STEP(MD5_F3, d, a, b, c, in[12] + 0xe6db99e5, 11);
    MD5STEP(MD5_F3, c, d, a, b, in[15] + 0x1fa27cf8, 16);
    MD5STEP(MD5_F3, b, c, d, a, in[2] + 0xc4ac5665, 23);

    MD5STEP(MD5_F4, a, b, c, d, in[0] + 0xf4292244, 6);
    MD5STEP(MD5_F4, d, a, b, c, in[7] + 0x432aff97, 10);
    MD5STEP(MD5_F4, c, d, a, b, in[14] + 0xab9423a7, 15);
    MD5STEP(MD5_F4, b, c, d, a, in[5] + 0xfc93a039, 21);
    MD5STEP(MD5_F4, a, b, c, d, in[12] + 0x655b59c3, 6);
    MD5STEP(MD5_F4, d, a, b, c, in[3] + 0x8f0ccc92, 10);
    MD5STEP(MD5_F4, c, d, a, b, in[10] + 0xffeff47d, 15);
    MD5STEP(MD5_F4, b, c, d, a, in[1] + 0x85845dd1, 21);
    MD5STEP(MD5_F4, a, b, c, d, in[8] + 0x6fa87e4f, 6);
    MD5STEP(MD5_F4, d, a, b, c, in[15] + 0xfe2ce6e0, 10);
    MD5STEP(MD5_F4, c, d, a, b, in[6] + 0xa3014314, 15);
    MD5STEP(MD5_F4, b, c, d, a, in[13] + 0x4e0811a1, 21);
    MD5STEP(MD5_F4, a, b, c, d, in[4] + 0xf7537e82, 6);
    MD5STEP(MD5_F4, d, a, b, c, in[11] + 0xbd3af235, 10);
    MD5STEP(MD5_F4, c, d, a, b, in[2] + 0x2ad7d2bb, 15);
    MD5STEP(MD5_F4, b, c, d, a, in[9] + 0xeb86d391, 21);

    buf[0] += a;
    buf[1] += b;
    buf[2] += c;
    buf[3] += d;
}

/*
 * Update context to reflect the concatenation of another buffer full
 * of bytes.
 */
inline static void MD5Update(struct MD5Context *ctx, const char *buf, unsigned len)
{
    uint32 t;

    /* Update bitcount */

    t = ctx->bits[0];
    if ((ctx->bits[0] = t + ((uint32) len << 3)) < t)
	ctx->bits[1]++; 	/* Carry from low to high */
    ctx->bits[1] += len >> 29;

    t = (t >> 3) & 0x3f;	/* Bytes already in shsInfo->data */

    /* Handle any leading odd-sized chunks */

    if (t) {
	unsigned char *p = (unsigned char *) ctx->in + t;

	t = 64 - t;
	if (len < t) {
	    memcpy(p, buf, len);
	    return;
	}
	memcpy(p, buf, t);
	MD5_byteReverse(ctx->in, 16);
	MD5Transform(ctx->buf, (uint32 *) ctx->in);
	buf += t;
	len -= t;
    }
    /* Process data in 64-byte chunks */

    while (len >= 64) {
	memcpy(ctx->in, buf, 64);
	MD5_byteReverse(ctx->in, 16);
	MD5Transform(ctx->buf, (uint32 *) ctx->in);
	buf += 64;
	len -= 64;
    }

    /* Handle any remaining bytes of data. */

    memcpy(ctx->in, buf, len);
}

/*
 * Final wrapup - pad to 64-byte boundary with the bit pattern 
 * 1 0* (64-bit count of bits processed, MSB-first)
 */
inline static void MD5Final(unsigned char digest[16], struct MD5Context *ctx)
{
    unsigned count;
    unsigned char *p;

    /* Compute number of bytes mod 64 */
    count = (ctx->bits[0] >> 3) & 0x3F;

    /* Set the first char of padding to 0x80.  This is safe since there is
       always at least one byte free */
    p = ctx->in + count;
    *p++ = 0x80;

    /* Bytes of padding needed to make 64 bytes */
    count = 64 - 1 - count;

    /* Pad out to 56 mod 64 */
    if (count < 8) {
	/* Two lots of padding:  Pad the first block to 64 bytes */
	memset(p, 0, count);
	MD5_byteReverse(ctx->in, 16);
	MD5Transform(ctx->buf, (uint32 *) ctx->in);

	/* Now fill the next block with 56 bytes */
	memset(ctx->in, 0, 56);
    } else {
	/* Pad block to 56 bytes */
	memset(p, 0, count - 8);
    }
    MD5_byteReverse(ctx->in, 14);

    /* Append length in bits and transform */
    ((uint32 *) ctx->in)[14] = ctx->bits[0];
    ((uint32 *) ctx->in)[15] = ctx->bits[1];

    MD5Transform(ctx->buf, (uint32 *) ctx->in);
    MD5_byteReverse((unsigned char *) ctx->buf, 4);
    memcpy(digest, ctx->buf, 16);
    memset(ctx, 0, sizeof(*ctx));        /* In case it's sensitive */
}

/*  Define CHECK_HARDWARE_PROPERTIES to have main.c verify
    byte order and uint32 settings.  */
#define CHECK_HARDWARE_PROPERTIES

#endif /* !MD5_H */