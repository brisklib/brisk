/* LibTomCrypt, modular cryptographic library -- Tom St Denis */
/* SPDX-License-Identifier: Unlicense */
#include "private.h"

#define F0(x, y, z) (z ^ (x & (y ^ z)))
#define F1(x, y, z) (x ^ y ^ z)
#define F2(x, y, z) ((x & y) | (z & (x | y)))
#define F3(x, y, z) (x ^ y ^ z)

#ifdef LTC_CLEAN_STACK
static int ss_sha1_compress(hash_state* md, const unsigned char* buf)
#else
static int s_sha1_compress(hash_state* md, const unsigned char* buf)
#endif
{
    ulong32 a, b, c, d, e, W[80], i;
#ifdef LTC_SMALL_CODE
    ulong32 t;
#endif

    /* copy the state into 512-bits into W[0..15] */
    for (i = 0; i < 16; i++) {
        LOAD32H(W[i], buf + (4 * i));
    }

    /* copy state */
    a = md->sha1.state[0];
    b = md->sha1.state[1];
    c = md->sha1.state[2];
    d = md->sha1.state[3];
    e = md->sha1.state[4];

    /* expand it */
    for (i = 16; i < 80; i++) {
        W[i] = ROL(W[i - 3] ^ W[i - 8] ^ W[i - 14] ^ W[i - 16], 1);
    }

/* compress */
/* round one */
#define FF0(a, b, c, d, e, i)                                                                                \
    e = (ROLc(a, 5) + F0(b, c, d) + e + W[i] + 0x5a827999UL);                                                \
    b = ROLc(b, 30);
#define FF1(a, b, c, d, e, i)                                                                                \
    e = (ROLc(a, 5) + F1(b, c, d) + e + W[i] + 0x6ed9eba1UL);                                                \
    b = ROLc(b, 30);
#define FF2(a, b, c, d, e, i)                                                                                \
    e = (ROLc(a, 5) + F2(b, c, d) + e + W[i] + 0x8f1bbcdcUL);                                                \
    b = ROLc(b, 30);
#define FF3(a, b, c, d, e, i)                                                                                \
    e = (ROLc(a, 5) + F3(b, c, d) + e + W[i] + 0xca62c1d6UL);                                                \
    b = ROLc(b, 30);

#ifdef LTC_SMALL_CODE

    for (i = 0; i < 20;) {
        FF0(a, b, c, d, e, i++);
        t = e;
        e = d;
        d = c;
        c = b;
        b = a;
        a = t;
    }

    for (; i < 40;) {
        FF1(a, b, c, d, e, i++);
        t = e;
        e = d;
        d = c;
        c = b;
        b = a;
        a = t;
    }

    for (; i < 60;) {
        FF2(a, b, c, d, e, i++);
        t = e;
        e = d;
        d = c;
        c = b;
        b = a;
        a = t;
    }

    for (; i < 80;) {
        FF3(a, b, c, d, e, i++);
        t = e;
        e = d;
        d = c;
        c = b;
        b = a;
        a = t;
    }

#else

    for (i = 0; i < 20;) {
        FF0(a, b, c, d, e, i++);
        FF0(e, a, b, c, d, i++);
        FF0(d, e, a, b, c, i++);
        FF0(c, d, e, a, b, i++);
        FF0(b, c, d, e, a, i++);
    }

    /* round two */
    for (; i < 40;) {
        FF1(a, b, c, d, e, i++);
        FF1(e, a, b, c, d, i++);
        FF1(d, e, a, b, c, i++);
        FF1(c, d, e, a, b, i++);
        FF1(b, c, d, e, a, i++);
    }

    /* round three */
    for (; i < 60;) {
        FF2(a, b, c, d, e, i++);
        FF2(e, a, b, c, d, i++);
        FF2(d, e, a, b, c, i++);
        FF2(c, d, e, a, b, i++);
        FF2(b, c, d, e, a, i++);
    }

    /* round four */
    for (; i < 80;) {
        FF3(a, b, c, d, e, i++);
        FF3(e, a, b, c, d, i++);
        FF3(d, e, a, b, c, i++);
        FF3(c, d, e, a, b, i++);
        FF3(b, c, d, e, a, i++);
    }
#endif

#undef FF0
#undef FF1
#undef FF2
#undef FF3

    /* store */
    md->sha1.state[0] = md->sha1.state[0] + a;
    md->sha1.state[1] = md->sha1.state[1] + b;
    md->sha1.state[2] = md->sha1.state[2] + c;
    md->sha1.state[3] = md->sha1.state[3] + d;
    md->sha1.state[4] = md->sha1.state[4] + e;

    return CRYPT_OK;
}

#ifdef LTC_CLEAN_STACK
static int s_sha1_compress(hash_state* md, const unsigned char* buf) {
    int err;
    err = ss_sha1_compress(md, buf);
    burn_stack(sizeof(ulong32) * 87);
    return err;
}
#endif

/**
   Initialize the hash state
   @param md   The hash state you wish to initialize
   @return CRYPT_OK if successful
*/
int PUB_NAME(sha1_init)(hash_state* md) {
    LTC_ARGCHK(md != NULL);
    md->sha1.state[0] = 0x67452301UL;
    md->sha1.state[1] = 0xefcdab89UL;
    md->sha1.state[2] = 0x98badcfeUL;
    md->sha1.state[3] = 0x10325476UL;
    md->sha1.state[4] = 0xc3d2e1f0UL;
    md->sha1.curlen   = 0;
    md->sha1.length   = 0;
    return CRYPT_OK;
}

/**
   Process a block of memory though the hash
   @param md     The hash state
   @param in     The data to hash
   @param inlen  The length of the data (octets)
   @return CRYPT_OK if successful
*/
HASH_PROCESS(PUB_NAME(sha1_process), s_sha1_compress, sha1, 64)

/**
   Terminate the hash to get the digest
   @param md  The hash state
   @param out [out] The destination of the hash (20 bytes)
   @return CRYPT_OK if successful
*/
int PUB_NAME(sha1_done)(hash_state* md, unsigned char* out) {
    int i;

    LTC_ARGCHK(md != NULL);
    LTC_ARGCHK(out != NULL);

    if (md->sha1.curlen >= sizeof(md->sha1.buf)) {
        return CRYPT_INVALID_ARG;
    }

    /* increase the length of the message */
    md->sha1.length += md->sha1.curlen * 8;

    /* append the '1' bit */
    md->sha1.buf[md->sha1.curlen++] = (unsigned char)0x80;

    /* if the length is currently above 56 bytes we append zeros
     * then compress.  Then we can fall back to padding zeros and length
     * encoding like normal.
     */
    if (md->sha1.curlen > 56) {
        while (md->sha1.curlen < 64) {
            md->sha1.buf[md->sha1.curlen++] = (unsigned char)0;
        }
        s_sha1_compress(md, md->sha1.buf);
        md->sha1.curlen = 0;
    }

    /* pad upto 56 bytes of zeroes */
    while (md->sha1.curlen < 56) {
        md->sha1.buf[md->sha1.curlen++] = (unsigned char)0;
    }

    /* store length */
    STORE64H(md->sha1.length, md->sha1.buf + 56);
    s_sha1_compress(md, md->sha1.buf);

    /* copy output */
    for (i = 0; i < 5; i++) {
        STORE32H(md->sha1.state[i], out + (4 * i));
    }
#ifdef LTC_CLEAN_STACK
    zeromem(md, sizeof(hash_state));
#endif
    return CRYPT_OK;
}
