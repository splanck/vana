/*
 * BSD 2-Clause License: Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that the copyright notice and
 * this permission notice appear in all copies. This software is provided "as is"
 * without warranty.
 *
 * Purpose: DES crypt implementation for vlibc.
 *
 * Copyright (c) 2025
 */

/*
 * FreeSec: libcrypt for NetBSD
 *
 * Copyright (c) 1994 David Burren
 * All rights reserved.
 *
 * Adapted for FreeBSD-2.0 by Geoffrey M. Rehmet
 *	this file should now *only* export crypt(), in order to make
 *	binaries of libcrypt exportable from the USA
 *
 * Adapted for FreeBSD-4.0 by Mark R V Murray
 *	this file should now *only* export crypt_des(), in order to make
 *	a module that can be optionally included in libcrypt.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the author nor the names of other contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This is an original implementation of the DES and the crypt(3) interfaces
 * by David Burren <davidb@werj.com.au>.
 *
 * An excellent reference on the underlying algorithm (and related
 * algorithms) is:
 *
 *	B. Schneier, Applied Cryptography: protocols, algorithms,
 *	and source code in C, John Wiley & Sons, 1994.
 *
 * Note that in that book's description of DES the lookups for the initial,
 * pbox, and final permutations are inverted (this has been brought to the
 * attention of the author).  A list of errata for this book has been
 * posted to the sci.crypt newsgroup by the author and is available for FTP.
 *
 * ARCHITECTURE ASSUMPTIONS:
 *	It is assumed that the 8-byte arrays passed by reference can be
 *	addressed as arrays of u_int32_t's (ie. the CPU is not picky about
 *	alignment).
 */

#include <sys/cdefs.h>
#include <sys/types.h>
#include <sys/param.h>
#include <arpa/inet.h>
#include <pwd.h>
#include <string.h>
#include "stdio.h"
#include "stdlib.h"
#include <alloca.h>
#include <openssl/evp.h>
#include <openssl/crypto.h>
#include "crypt.h"

#ifndef u_char
typedef unsigned char u_char;
#endif
#ifndef _PASSWORD_EFMT1
#define _PASSWORD_EFMT1 '_'
#endif
#ifndef u_long
typedef unsigned long u_long;
#endif

/* We can't always assume gcc */
#if	defined(__GNUC__) && !defined(lint)
#define INLINE inline
#else
#define INLINE
#endif


static const u_char	IP[64] = {
	58, 50, 42, 34, 26, 18, 10,  2, 60, 52, 44, 36, 28, 20, 12,  4,
	62, 54, 46, 38, 30, 22, 14,  6, 64, 56, 48, 40, 32, 24, 16,  8,
	57, 49, 41, 33, 25, 17,  9,  1, 59, 51, 43, 35, 27, 19, 11,  3,
	61, 53, 45, 37, 29, 21, 13,  5, 63, 55, 47, 39, 31, 23, 15,  7
};

static __thread u_char	inv_key_perm[64];
static const u_char	key_perm[56] = {
	57, 49, 41, 33, 25, 17,  9,  1, 58, 50, 42, 34, 26, 18,
	10,  2, 59, 51, 43, 35, 27, 19, 11,  3, 60, 52, 44, 36,
	63, 55, 47, 39, 31, 23, 15,  7, 62, 54, 46, 38, 30, 22,
	14,  6, 61, 53, 45, 37, 29, 21, 13,  5, 28, 20, 12,  4
};

static const u_char	key_shifts[16] = {
	1, 1, 2, 2, 2, 2, 2, 2, 1, 2, 2, 2, 2, 2, 2, 1
};

static __thread u_char	inv_comp_perm[56];
static const u_char	comp_perm[48] = {
	14, 17, 11, 24,  1,  5,  3, 28, 15,  6, 21, 10,
	23, 19, 12,  4, 26,  8, 16,  7, 27, 20, 13,  2,
	41, 52, 31, 37, 47, 55, 30, 40, 51, 45, 33, 48,
	44, 49, 39, 56, 34, 53, 46, 42, 50, 36, 29, 32
};

/*
 *	No E box is used, as it's replaced by some ANDs, shifts, and ORs.
 */

static __thread u_char	u_sbox[8][64];
static const u_char	sbox[8][64] = {
	{
		14,  4, 13,  1,  2, 15, 11,  8,  3, 10,  6, 12,  5,  9,  0,  7,
		 0, 15,  7,  4, 14,  2, 13,  1, 10,  6, 12, 11,  9,  5,  3,  8,
		 4,  1, 14,  8, 13,  6,  2, 11, 15, 12,  9,  7,  3, 10,  5,  0,
		15, 12,  8,  2,  4,  9,  1,  7,  5, 11,  3, 14, 10,  0,  6, 13
	},
	{
		15,  1,  8, 14,  6, 11,  3,  4,  9,  7,  2, 13, 12,  0,  5, 10,
		 3, 13,  4,  7, 15,  2,  8, 14, 12,  0,  1, 10,  6,  9, 11,  5,
		 0, 14,  7, 11, 10,  4, 13,  1,  5,  8, 12,  6,  9,  3,  2, 15,
		13,  8, 10,  1,  3, 15,  4,  2, 11,  6,  7, 12,  0,  5, 14,  9
	},
	{
		10,  0,  9, 14,  6,  3, 15,  5,  1, 13, 12,  7, 11,  4,  2,  8,
		13,  7,  0,  9,  3,  4,  6, 10,  2,  8,  5, 14, 12, 11, 15,  1,
		13,  6,  4,  9,  8, 15,  3,  0, 11,  1,  2, 12,  5, 10, 14,  7,
		 1, 10, 13,  0,  6,  9,  8,  7,  4, 15, 14,  3, 11,  5,  2, 12
	},
	{
		 7, 13, 14,  3,  0,  6,  9, 10,  1,  2,  8,  5, 11, 12,  4, 15,
		13,  8, 11,  5,  6, 15,  0,  3,  4,  7,  2, 12,  1, 10, 14,  9,
		10,  6,  9,  0, 12, 11,  7, 13, 15,  1,  3, 14,  5,  2,  8,  4,
		 3, 15,  0,  6, 10,  1, 13,  8,  9,  4,  5, 11, 12,  7,  2, 14
	},
	{
		 2, 12,  4,  1,  7, 10, 11,  6,  8,  5,  3, 15, 13,  0, 14,  9,
		14, 11,  2, 12,  4,  7, 13,  1,  5,  0, 15, 10,  3,  9,  8,  6,
		 4,  2,  1, 11, 10, 13,  7,  8, 15,  9, 12,  5,  6,  3,  0, 14,
		11,  8, 12,  7,  1, 14,  2, 13,  6, 15,  0,  9, 10,  4,  5,  3
	},
	{
		12,  1, 10, 15,  9,  2,  6,  8,  0, 13,  3,  4, 14,  7,  5, 11,
		10, 15,  4,  2,  7, 12,  9,  5,  6,  1, 13, 14,  0, 11,  3,  8,
		 9, 14, 15,  5,  2,  8, 12,  3,  7,  0,  4, 10,  1, 13, 11,  6,
		 4,  3,  2, 12,  9,  5, 15, 10, 11, 14,  1,  7,  6,  0,  8, 13
	},
	{
		 4, 11,  2, 14, 15,  0,  8, 13,  3, 12,  9,  7,  5, 10,  6,  1,
		13,  0, 11,  7,  4,  9,  1, 10, 14,  3,  5, 12,  2, 15,  8,  6,
		 1,  4, 11, 13, 12,  3,  7, 14, 10, 15,  6,  8,  0,  5,  9,  2,
		 6, 11, 13,  8,  1,  4, 10,  7,  9,  5,  0, 15, 14,  2,  3, 12
	},
	{
		13,  2,  8,  4,  6, 15, 11,  1, 10,  9,  3, 14,  5,  0, 12,  7,
		 1, 15, 13,  8, 10,  3,  7,  4, 12,  5,  6, 11,  0, 14,  9,  2,
		 7, 11,  4,  1,  9, 12, 14,  2,  0,  6, 10, 13, 15,  3,  5,  8,
		 2,  1, 14,  7,  4, 10,  8, 13, 15, 12,  9,  0,  3,  5,  6, 11
	}
};

static __thread u_char	un_pbox[32];
static const u_char	pbox[32] = {
	16,  7, 20, 21, 29, 12, 28, 17,  1, 15, 23, 26,  5, 18, 31, 10,
	 2,  8, 24, 14, 32, 27,  3,  9, 19, 13, 30,  6, 22, 11,  4, 25
};

static const u_int32_t	bits32[32] =
{
	0x80000000, 0x40000000, 0x20000000, 0x10000000,
	0x08000000, 0x04000000, 0x02000000, 0x01000000,
	0x00800000, 0x00400000, 0x00200000, 0x00100000,
	0x00080000, 0x00040000, 0x00020000, 0x00010000,
	0x00008000, 0x00004000, 0x00002000, 0x00001000,
	0x00000800, 0x00000400, 0x00000200, 0x00000100,
	0x00000080, 0x00000040, 0x00000020, 0x00000010,
	0x00000008, 0x00000004, 0x00000002, 0x00000001
};

static const u_char	bits8[8] = { 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01 };

static __thread u_int32_t	saltbits;
static __thread u_int32_t	old_salt;
static __thread const u_int32_t	*bits28, *bits24;
static __thread u_char		init_perm[64], final_perm[64];
static __thread u_int32_t	en_keysl[16], en_keysr[16];
static __thread u_int32_t	de_keysl[16], de_keysr[16];
static __thread int		des_initialised = 0;
static __thread u_char		m_sbox[4][4096];
static __thread u_int32_t	psbox[4][256];
static __thread u_int32_t	ip_maskl[8][256], ip_maskr[8][256];
static __thread u_int32_t	fp_maskl[8][256], fp_maskr[8][256];
static __thread u_int32_t	key_perm_maskl[8][128], key_perm_maskr[8][128];
static __thread u_int32_t	comp_maskl[8][128], comp_maskr[8][128];
static __thread u_int32_t	old_rawkey0, old_rawkey1;

static const u_char	ascii64[] =
	 "./0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
/*	  0000000000111111111122222222223333333333444444444455555555556666 */
/*	  0123456789012345678901234567890123456789012345678901234567890123 */

/*
 * Encode the low-order bits of v into n characters of the supplied
 * buffer using the custom base-64 alphabet.  The buffer is not
 * null-terminated by this routine.
 */
static void
_crypt_to64(char *s, u_long v, int n)
{
        while (n-- > 0) {
                *s++ = ascii64[v & 0x3f];
                v >>= 6;
        }
}

/*
 * Convert three bytes into base-64 text.  'n' specifies how many output
 * characters to generate and *cp is advanced past the written data.
 */
static void
b64_from_24bit(unsigned int b2, unsigned int b1, unsigned int b0, int n,
    char **cp)
{
        unsigned int w = (b2 << 16) | (b1 << 8) | b0;
        while (n-- > 0) {
                **cp = ascii64[w & 0x3f];
                (*cp)++;
                w >>= 6;
        }
}

/*
 * Translate a character from the custom base-64 alphabet back into the
 * corresponding 6-bit value.  Invalid characters return zero.
 */
static INLINE int
ascii_to_bin(char ch)
{
	if (ch > 'z')
		return(0);
	if (ch >= 'a')
		return(ch - 'a' + 38);
	if (ch > 'Z')
		return(0);
	if (ch >= 'A')
		return(ch - 'A' + 12);
	if (ch > '9')
		return(0);
	if (ch >= '.')
		return(ch - '.');
	return(0);
}

/*
 * Prepare DES lookup tables and masks.  This must be called before any
 * DES operations are performed and only needs to run once per thread.
 */
static void
des_init(void)
{
	int	i, j, b, k, inbit, obit;
	u_int32_t	*p, *il, *ir, *fl, *fr;

	old_rawkey0 = old_rawkey1 = 0L;
	saltbits = 0L;
	old_salt = 0L;
	bits24 = (bits28 = bits32 + 4) + 4;

	/*
	 * Invert the S-boxes, reordering the input bits.
	 */
	for (i = 0; i < 8; i++)
		for (j = 0; j < 64; j++) {
			b = (j & 0x20) | ((j & 1) << 4) | ((j >> 1) & 0xf);
			u_sbox[i][j] = sbox[i][b];
		}

	/*
	 * Convert the inverted S-boxes into 4 arrays of 8 bits.
	 * Each will handle 12 bits of the S-box input.
	 */
	for (b = 0; b < 4; b++)
		for (i = 0; i < 64; i++)
			for (j = 0; j < 64; j++)
				m_sbox[b][(i << 6) | j] =
					(u_char)((u_sbox[(b << 1)][i] << 4) |
					u_sbox[(b << 1) + 1][j]);

	/*
	 * Set up the initial & final permutations into a useful form, and
	 * initialise the inverted key permutation.
	 */
	for (i = 0; i < 64; i++) {
		init_perm[final_perm[i] = IP[i] - 1] = (u_char)i;
		inv_key_perm[i] = 255;
	}

	/*
	 * Invert the key permutation and initialise the inverted key
	 * compression permutation.
	 */
	for (i = 0; i < 56; i++) {
		inv_key_perm[key_perm[i] - 1] = (u_char)i;
		inv_comp_perm[i] = 255;
	}

	/*
	 * Invert the key compression permutation.
	 */
	for (i = 0; i < 48; i++) {
		inv_comp_perm[comp_perm[i] - 1] = (u_char)i;
	}

	/*
	 * Set up the OR-mask arrays for the initial and final permutations,
	 * and for the key initial and compression permutations.
	 */
	for (k = 0; k < 8; k++) {
		for (i = 0; i < 256; i++) {
			*(il = &ip_maskl[k][i]) = 0L;
			*(ir = &ip_maskr[k][i]) = 0L;
			*(fl = &fp_maskl[k][i]) = 0L;
			*(fr = &fp_maskr[k][i]) = 0L;
			for (j = 0; j < 8; j++) {
				inbit = 8 * k + j;
				if (i & bits8[j]) {
					if ((obit = init_perm[inbit]) < 32)
						*il |= bits32[obit];
					else
						*ir |= bits32[obit-32];
					if ((obit = final_perm[inbit]) < 32)
						*fl |= bits32[obit];
					else
						*fr |= bits32[obit - 32];
				}
			}
		}
		for (i = 0; i < 128; i++) {
			*(il = &key_perm_maskl[k][i]) = 0L;
			*(ir = &key_perm_maskr[k][i]) = 0L;
			for (j = 0; j < 7; j++) {
				inbit = 8 * k + j;
				if (i & bits8[j + 1]) {
					if ((obit = inv_key_perm[inbit]) == 255)
						continue;
					if (obit < 28)
						*il |= bits28[obit];
					else
						*ir |= bits28[obit - 28];
				}
			}
			*(il = &comp_maskl[k][i]) = 0L;
			*(ir = &comp_maskr[k][i]) = 0L;
			for (j = 0; j < 7; j++) {
				inbit = 7 * k + j;
				if (i & bits8[j + 1]) {
					if ((obit=inv_comp_perm[inbit]) == 255)
						continue;
					if (obit < 24)
						*il |= bits24[obit];
					else
						*ir |= bits24[obit - 24];
				}
			}
		}
	}

	/*
	 * Invert the P-box permutation, and convert into OR-masks for
	 * handling the output of the S-box arrays setup above.
	 */
	for (i = 0; i < 32; i++)
		un_pbox[pbox[i] - 1] = (u_char)i;

	for (b = 0; b < 4; b++)
		for (i = 0; i < 256; i++) {
			*(p = &psbox[b][i]) = 0L;
			for (j = 0; j < 8; j++) {
				if (i & bits8[j])
					*p |= bits32[un_pbox[8 * b + j]];
			}
		}

	des_initialised = 1;
}

/*
 * Precompute bit patterns used for salting the DES rounds.  The salt
 * value should contain up to 24 useful bits.  Subsequent operations use
 * the global saltbits prepared here.
 */
static void
setup_salt(u_int32_t salt)
{
	u_int32_t	obit, saltbit;
	int		i;

	if (salt == old_salt)
		return;
	old_salt = salt;

	saltbits = 0L;
	saltbit = 1;
	obit = 0x800000;
	for (i = 0; i < 24; i++) {
		if (salt & saltbit)
			saltbits |= obit;
		saltbit <<= 1;
		obit >>= 1;
	}
}

/*
 * Expand the supplied 64-bit key and compute the DES key schedule.
 * Returns 0 on success.  Reuses cached results when the key is unchanged.
 */
static int
des_setkey(const char *key)
{
	u_int32_t	k0, k1, rawkey0, rawkey1;
	int		shifts, round;

	if (!des_initialised)
		des_init();

	rawkey0 = ntohl(*(const u_int32_t *) key);
	rawkey1 = ntohl(*(const u_int32_t *) (key + 4));

	if ((rawkey0 | rawkey1)
	    && rawkey0 == old_rawkey0
	    && rawkey1 == old_rawkey1) {
		/*
		 * Already setup for this key.
		 * This optimisation fails on a zero key (which is weak and
		 * has bad parity anyway) in order to simplify the starting
		 * conditions.
		 */
		return(0);
	}
	old_rawkey0 = rawkey0;
	old_rawkey1 = rawkey1;

	/*
	 *	Do key permutation and split into two 28-bit subkeys.
	 */
	k0 = key_perm_maskl[0][rawkey0 >> 25]
	   | key_perm_maskl[1][(rawkey0 >> 17) & 0x7f]
	   | key_perm_maskl[2][(rawkey0 >> 9) & 0x7f]
	   | key_perm_maskl[3][(rawkey0 >> 1) & 0x7f]
	   | key_perm_maskl[4][rawkey1 >> 25]
	   | key_perm_maskl[5][(rawkey1 >> 17) & 0x7f]
	   | key_perm_maskl[6][(rawkey1 >> 9) & 0x7f]
	   | key_perm_maskl[7][(rawkey1 >> 1) & 0x7f];
	k1 = key_perm_maskr[0][rawkey0 >> 25]
	   | key_perm_maskr[1][(rawkey0 >> 17) & 0x7f]
	   | key_perm_maskr[2][(rawkey0 >> 9) & 0x7f]
	   | key_perm_maskr[3][(rawkey0 >> 1) & 0x7f]
	   | key_perm_maskr[4][rawkey1 >> 25]
	   | key_perm_maskr[5][(rawkey1 >> 17) & 0x7f]
	   | key_perm_maskr[6][(rawkey1 >> 9) & 0x7f]
	   | key_perm_maskr[7][(rawkey1 >> 1) & 0x7f];
	/*
	 *	Rotate subkeys and do compression permutation.
	 */
	shifts = 0;
	for (round = 0; round < 16; round++) {
		u_int32_t	t0, t1;

		shifts += key_shifts[round];

		t0 = (k0 << shifts) | (k0 >> (28 - shifts));
		t1 = (k1 << shifts) | (k1 >> (28 - shifts));

		de_keysl[15 - round] =
		en_keysl[round] = comp_maskl[0][(t0 >> 21) & 0x7f]
				| comp_maskl[1][(t0 >> 14) & 0x7f]
				| comp_maskl[2][(t0 >> 7) & 0x7f]
				| comp_maskl[3][t0 & 0x7f]
				| comp_maskl[4][(t1 >> 21) & 0x7f]
				| comp_maskl[5][(t1 >> 14) & 0x7f]
				| comp_maskl[6][(t1 >> 7) & 0x7f]
				| comp_maskl[7][t1 & 0x7f];

		de_keysr[15 - round] =
		en_keysr[round] = comp_maskr[0][(t0 >> 21) & 0x7f]
				| comp_maskr[1][(t0 >> 14) & 0x7f]
				| comp_maskr[2][(t0 >> 7) & 0x7f]
				| comp_maskr[3][t0 & 0x7f]
				| comp_maskr[4][(t1 >> 21) & 0x7f]
				| comp_maskr[5][(t1 >> 14) & 0x7f]
				| comp_maskr[6][(t1 >> 7) & 0x7f]
				| comp_maskr[7][t1 & 0x7f];
	}
	return(0);
}

/*
 * Core DES engine.  Encrypts or decrypts a 64-bit block depending on the
 * sign of "count".  Results are written to l_out and r_out.  Returning 1
 * indicates no operation when count is zero.
 */
static int
do_des(	u_int32_t l_in, u_int32_t r_in, u_int32_t *l_out, u_int32_t *r_out, int count)
{
	/*
	 *	l_in, r_in, l_out, and r_out are in pseudo-"big-endian" format.
	 */
	u_int32_t	l, r, *kl, *kr, *kl1, *kr1;
	u_int32_t	f, r48l, r48r;
	int		round;

	if (count == 0) {
		return(1);
	} else if (count > 0) {
		/*
		 * Encrypting
		 */
		kl1 = en_keysl;
		kr1 = en_keysr;
	} else {
		/*
		 * Decrypting
		 */
		count = -count;
		kl1 = de_keysl;
		kr1 = de_keysr;
	}

	/*
	 *	Do initial permutation (IP).
	 */
	l = ip_maskl[0][l_in >> 24]
	  | ip_maskl[1][(l_in >> 16) & 0xff]
	  | ip_maskl[2][(l_in >> 8) & 0xff]
	  | ip_maskl[3][l_in & 0xff]
	  | ip_maskl[4][r_in >> 24]
	  | ip_maskl[5][(r_in >> 16) & 0xff]
	  | ip_maskl[6][(r_in >> 8) & 0xff]
	  | ip_maskl[7][r_in & 0xff];
	r = ip_maskr[0][l_in >> 24]
	  | ip_maskr[1][(l_in >> 16) & 0xff]
	  | ip_maskr[2][(l_in >> 8) & 0xff]
	  | ip_maskr[3][l_in & 0xff]
	  | ip_maskr[4][r_in >> 24]
	  | ip_maskr[5][(r_in >> 16) & 0xff]
	  | ip_maskr[6][(r_in >> 8) & 0xff]
	  | ip_maskr[7][r_in & 0xff];

	while (count--) {
		/*
		 * Do each round.
		 */
		kl = kl1;
		kr = kr1;
		round = 16;
		while (round--) {
			/*
			 * Expand R to 48 bits (simulate the E-box).
			 */
			r48l	= ((r & 0x00000001) << 23)
				| ((r & 0xf8000000) >> 9)
				| ((r & 0x1f800000) >> 11)
				| ((r & 0x01f80000) >> 13)
				| ((r & 0x001f8000) >> 15);

			r48r	= ((r & 0x0001f800) << 7)
				| ((r & 0x00001f80) << 5)
				| ((r & 0x000001f8) << 3)
				| ((r & 0x0000001f) << 1)
				| ((r & 0x80000000) >> 31);
			/*
			 * Do salting for crypt() and friends, and
			 * XOR with the permuted key.
			 */
			f = (r48l ^ r48r) & saltbits;
			r48l ^= f ^ *kl++;
			r48r ^= f ^ *kr++;
			/*
			 * Do sbox lookups (which shrink it back to 32 bits)
			 * and do the pbox permutation at the same time.
			 */
			f = psbox[0][m_sbox[0][r48l >> 12]]
			  | psbox[1][m_sbox[1][r48l & 0xfff]]
			  | psbox[2][m_sbox[2][r48r >> 12]]
			  | psbox[3][m_sbox[3][r48r & 0xfff]];
			/*
			 * Now that we've permuted things, complete f().
			 */
			f ^= l;
			l = r;
			r = f;
		}
		r = l;
		l = f;
	}
	/*
	 * Do final permutation (inverse of IP).
	 */
	*l_out	= fp_maskl[0][l >> 24]
		| fp_maskl[1][(l >> 16) & 0xff]
		| fp_maskl[2][(l >> 8) & 0xff]
		| fp_maskl[3][l & 0xff]
		| fp_maskl[4][r >> 24]
		| fp_maskl[5][(r >> 16) & 0xff]
		| fp_maskl[6][(r >> 8) & 0xff]
		| fp_maskl[7][r & 0xff];
	*r_out	= fp_maskr[0][l >> 24]
		| fp_maskr[1][(l >> 16) & 0xff]
		| fp_maskr[2][(l >> 8) & 0xff]
		| fp_maskr[3][l & 0xff]
		| fp_maskr[4][r >> 24]
		| fp_maskr[5][(r >> 16) & 0xff]
		| fp_maskr[6][(r >> 8) & 0xff]
		| fp_maskr[7][r & 0xff];
	return(0);
}

/*
 * Public interface to the DES primitive.  Encrypts the 64-bit block at
 * 'in' into 'out' using the current key schedule and provided salt for
 * the specified number of rounds.
 */
static int
des_cipher(const char *in, char *out, u_long salt, int count)
{
	u_int32_t	l_out, r_out, rawl, rawr;
	int		retval;
	union {
		u_int32_t	*ui32;
		const char	*c;
	} trans;

	if (!des_initialised)
		des_init();

	setup_salt(salt);

	trans.c = in;
	rawl = ntohl(*trans.ui32++);
	rawr = ntohl(*trans.ui32);

	retval = do_des(rawl, rawr, &l_out, &r_out, count);

	trans.c = out;
	*trans.ui32++ = htonl(l_out);
	*trans.ui32 = htonl(r_out);
	return(retval);
}

/*
 * Classic DES password hashing routine.  'key' is the user's password,
 * 'setting' contains the salt and iteration count, and the resulting
 * hash is written as a NUL terminated string to 'buffer'.
 * Returns 0 on success or -1 on failure.
 */
int
crypt_des(const char *key, const char *setting, char *buffer)
{
	int		i;
	u_int32_t	count, salt, l, r0, r1, keybuf[2];
	u_char		*q;

	if (!des_initialised)
		des_init();

	/*
	 * Copy the key, shifting each character up by one bit
	 * and padding with zeros.
	 */
	q = (u_char *)keybuf;
	while (q - (u_char *)keybuf - 8) {
		*q++ = *key << 1;
		if (*key != '\0')
			key++;
	}
	if (des_setkey((char *)keybuf))
		return (-1);

	if (*setting == _PASSWORD_EFMT1) {
		/*
		 * "new"-style:
		 *	setting - underscore, 4 bytes of count, 4 bytes of salt
		 *	key - unlimited characters
		 */
		for (i = 1, count = 0L; i < 5; i++)
			count |= ascii_to_bin(setting[i]) << ((i - 1) * 6);

		for (i = 5, salt = 0L; i < 9; i++)
			salt |= ascii_to_bin(setting[i]) << ((i - 5) * 6);

		while (*key) {
			/*
			 * Encrypt the key with itself.
			 */
			if (des_cipher((char *)keybuf, (char *)keybuf, 0L, 1))
				return (-1);
			/*
			 * And XOR with the next 8 characters of the key.
			 */
			q = (u_char *)keybuf;
			while (q - (u_char *)keybuf - 8 && *key)
				*q++ ^= *key++ << 1;

			if (des_setkey((char *)keybuf))
				return (-1);
		}
		buffer = stpncpy(buffer, setting, 9);
	} else {
		/*
		 * "old"-style:
		 *	setting - 2 bytes of salt
		 *	key - up to 8 characters
		 */
		count = 25;

		salt = (ascii_to_bin(setting[1]) << 6)
		     |  ascii_to_bin(setting[0]);

		*buffer++ = setting[0];
		/*
		 * If the encrypted password that the salt was extracted from
		 * is only 1 character long, the salt will be corrupted.  We
		 * need to ensure that the output string doesn't have an extra
		 * NUL in it!
		 */
		*buffer++ = setting[1] ? setting[1] : setting[0];
	}
	setup_salt(salt);
	/*
	 * Do it.
	 */
	if (do_des(0L, 0L, &r0, &r1, (int)count))
		return (-1);
	/*
	 * Now encode the result...
	 */
	l = (r0 >> 8);
	*buffer++ = ascii64[(l >> 18) & 0x3f];
	*buffer++ = ascii64[(l >> 12) & 0x3f];
	*buffer++ = ascii64[(l >> 6) & 0x3f];
	*buffer++ = ascii64[l & 0x3f];

	l = (r0 << 16) | ((r1 >> 16) & 0xffff);
	*buffer++ = ascii64[(l >> 18) & 0x3f];
	*buffer++ = ascii64[(l >> 12) & 0x3f];
	*buffer++ = ascii64[(l >> 6) & 0x3f];
	*buffer++ = ascii64[l & 0x3f];

	l = r1 << 2;
	*buffer++ = ascii64[(l >> 12) & 0x3f];
	*buffer++ = ascii64[(l >> 6) & 0x3f];
	*buffer++ = ascii64[l & 0x3f];
       *buffer = '\0';

       return (0);
}

/*
 * MD5-based password hash used by many systems.  The key 'pw' and the
 * salt string are processed using the algorithm described in RFC 1321.
 * The resulting hash text is written to 'buffer'.
 */
static int
crypt_md5(const char *pw, const char *salt, char *buffer)
{
        EVP_MD_CTX *ctx, *ctx1;
        unsigned long l;
        int sl, pl;
        unsigned int i;
        unsigned char final[16];
        const char *ep;
        static const char *magic = "$1$";

        if (!strncmp(salt, magic, strlen(magic)))
                salt += strlen(magic);

        for (ep = salt; *ep && *ep != '$' && ep < salt + 8; ep++)
                continue;
        sl = ep - salt;

        if (!OPENSSL_init_crypto(0, NULL))
                return (-1);

        ctx = EVP_MD_CTX_new();
        ctx1 = EVP_MD_CTX_new();
        if (ctx == NULL || ctx1 == NULL)
                goto fail;

        if (EVP_DigestInit_ex(ctx, EVP_md5(), NULL) != 1)
                goto fail;
        EVP_DigestUpdate(ctx, pw, strlen(pw));
        EVP_DigestUpdate(ctx, magic, strlen(magic));
        EVP_DigestUpdate(ctx, salt, sl);

        if (EVP_DigestInit_ex(ctx1, EVP_md5(), NULL) != 1)
                goto fail;
        EVP_DigestUpdate(ctx1, pw, strlen(pw));
        EVP_DigestUpdate(ctx1, salt, sl);
        EVP_DigestUpdate(ctx1, pw, strlen(pw));
        EVP_DigestFinal_ex(ctx1, final, NULL);
        for (pl = strlen(pw); pl > 0; pl -= 16)
                EVP_DigestUpdate(ctx, final, pl > 16 ? 16 : pl);
        memset(final, 0, sizeof(final));

        for (i = strlen(pw); i; i >>= 1)
                if (i & 1)
                        EVP_DigestUpdate(ctx, final, 1);
                else
                        EVP_DigestUpdate(ctx, pw, 1);

        buffer = stpcpy(buffer, magic);
        buffer = stpncpy(buffer, salt, sl);
        *buffer++ = '$';

        EVP_DigestFinal_ex(ctx, final, NULL);

        for (i = 0; i < 1000; i++) {
                EVP_DigestInit_ex(ctx1, EVP_md5(), NULL);
                if (i & 1)
                        EVP_DigestUpdate(ctx1, pw, strlen(pw));
                else
                        EVP_DigestUpdate(ctx1, final, 16);
                if (i % 3)
                        EVP_DigestUpdate(ctx1, salt, sl);
                if (i % 7)
                        EVP_DigestUpdate(ctx1, pw, strlen(pw));
                if (i & 1)
                        EVP_DigestUpdate(ctx1, final, 16);
                else
                        EVP_DigestUpdate(ctx1, pw, strlen(pw));
                EVP_DigestFinal_ex(ctx1, final, NULL);
        }

        l = (final[0] << 16) | (final[6] << 8) | final[12];
        _crypt_to64(buffer, l, 4); buffer += 4;
        l = (final[1] << 16) | (final[7] << 8) | final[13];
        _crypt_to64(buffer, l, 4); buffer += 4;
        l = (final[2] << 16) | (final[8] << 8) | final[14];
        _crypt_to64(buffer, l, 4); buffer += 4;
        l = (final[3] << 16) | (final[9] << 8) | final[15];
        _crypt_to64(buffer, l, 4); buffer += 4;
        l = (final[4] << 16) | (final[10] << 8) | final[5];
        _crypt_to64(buffer, l, 4); buffer += 4;
        l = final[11];
        _crypt_to64(buffer, l, 2); buffer += 2;
        *buffer = '\0';

        memset(final, 0, sizeof(final));
        EVP_MD_CTX_free(ctx);
        EVP_MD_CTX_free(ctx1);
        return (0);

fail:
        if (ctx)
                EVP_MD_CTX_free(ctx);
        if (ctx1)
                EVP_MD_CTX_free(ctx1);
        return (-1);
}

/*
 * SHA-256 password hashing as used by modern Unix systems.  It supports an
 * optional rounds specification in the salt.  The final encoded hash is
 * written to 'buffer'.
 */
static int
crypt_sha256(const char *key, const char *salt, char *buffer)
{
        unsigned long srounds;
        uint8_t alt_result[32], temp_result[32];
        EVP_MD_CTX *ctx, *alt_ctx;
        size_t salt_len, key_len, cnt, rounds;
        char *cp, *p_bytes, *s_bytes, *endp;
        const char *num;
        int rounds_custom = 0;
        static const char prefix[] = "$5$";
        static const char rounds_prefix[] = "rounds=";

        rounds = 5000;

        if (strncmp(prefix, salt, sizeof(prefix) - 1) == 0)
                salt += sizeof(prefix) - 1;

        if (strncmp(salt, rounds_prefix, sizeof(rounds_prefix) - 1) == 0) {
                num = salt + sizeof(rounds_prefix) - 1;
                srounds = strtoul(num, &endp, 10);
                if (*endp == '$') {
                        salt = endp + 1;
                        if (srounds < 1000)
                                srounds = 1000;
                        else if (srounds > 999999999)
                                srounds = 999999999;
                        rounds = srounds;
                        rounds_custom = 1;
                }
        }

        salt_len = strcspn(salt, "$");
        if (salt_len > 16)
                salt_len = 16;
        key_len = strlen(key);

        if (!OPENSSL_init_crypto(0, NULL))
                return (-1);

        ctx = EVP_MD_CTX_new();
        alt_ctx = EVP_MD_CTX_new();
        if (ctx == NULL || alt_ctx == NULL)
                goto fail;

        if (EVP_DigestInit_ex(ctx, EVP_sha256(), NULL) != 1)
                goto fail;
        EVP_DigestUpdate(ctx, key, key_len);
        EVP_DigestUpdate(ctx, salt, salt_len);

        if (EVP_DigestInit_ex(alt_ctx, EVP_sha256(), NULL) != 1)
                goto fail;
        EVP_DigestUpdate(alt_ctx, key, key_len);
        EVP_DigestUpdate(alt_ctx, salt, salt_len);
        EVP_DigestUpdate(alt_ctx, key, key_len);
        EVP_DigestFinal_ex(alt_ctx, alt_result, NULL);

        for (cnt = key_len; cnt > 32; cnt -= 32)
                EVP_DigestUpdate(ctx, alt_result, 32);
        EVP_DigestUpdate(ctx, alt_result, cnt);

        for (cnt = key_len; cnt > 0; cnt >>= 1)
                if (cnt & 1)
                        EVP_DigestUpdate(ctx, alt_result, 32);
                else
                        EVP_DigestUpdate(ctx, key, key_len);

        EVP_DigestFinal_ex(ctx, alt_result, NULL);

        EVP_DigestInit_ex(alt_ctx, EVP_sha256(), NULL);
        for (cnt = 0; cnt < key_len; ++cnt)
                EVP_DigestUpdate(alt_ctx, key, key_len);
        EVP_DigestFinal_ex(alt_ctx, temp_result, NULL);

        cp = p_bytes = alloca(key_len);
        for (cnt = key_len; cnt >= 32; cnt -= 32) {
                memcpy(cp, temp_result, 32);
                cp += 32;
        }
        memcpy(cp, temp_result, cnt);

        EVP_DigestInit_ex(alt_ctx, EVP_sha256(), NULL);
        for (cnt = 0; cnt < (size_t)(16 + alt_result[0]); ++cnt)
                EVP_DigestUpdate(alt_ctx, salt, salt_len);
        EVP_DigestFinal_ex(alt_ctx, temp_result, NULL);

        cp = s_bytes = alloca(salt_len);
        for (cnt = salt_len; cnt >= 32; cnt -= 32) {
                memcpy(cp, temp_result, 32);
                cp += 32;
        }
        memcpy(cp, temp_result, cnt);

        for (cnt = 0; cnt < rounds; ++cnt) {
                EVP_DigestInit_ex(ctx, EVP_sha256(), NULL);
                if (cnt & 1)
                        EVP_DigestUpdate(ctx, p_bytes, key_len);
                else
                        EVP_DigestUpdate(ctx, alt_result, 32);
                if (cnt % 3)
                        EVP_DigestUpdate(ctx, s_bytes, salt_len);
                if (cnt % 7)
                        EVP_DigestUpdate(ctx, p_bytes, key_len);
                if (cnt & 1)
                        EVP_DigestUpdate(ctx, alt_result, 32);
                else
                        EVP_DigestUpdate(ctx, p_bytes, key_len);
                EVP_DigestFinal_ex(ctx, alt_result, NULL);
        }

        cp = stpcpy(buffer, prefix);
        if (rounds_custom)
                cp += sprintf(cp, "%s%zu$", rounds_prefix, rounds);
        cp = stpncpy(cp, salt, salt_len);
        *cp++ = '$';

        b64_from_24bit(alt_result[0], alt_result[10], alt_result[20], 4, &cp);
        b64_from_24bit(alt_result[21], alt_result[1], alt_result[11], 4, &cp);
        b64_from_24bit(alt_result[12], alt_result[22], alt_result[2], 4, &cp);
        b64_from_24bit(alt_result[3], alt_result[13], alt_result[23], 4, &cp);
        b64_from_24bit(alt_result[24], alt_result[4], alt_result[14], 4, &cp);
        b64_from_24bit(alt_result[15], alt_result[25], alt_result[5], 4, &cp);
        b64_from_24bit(alt_result[6], alt_result[16], alt_result[26], 4, &cp);
        b64_from_24bit(alt_result[27], alt_result[7], alt_result[17], 4, &cp);
        b64_from_24bit(alt_result[18], alt_result[28], alt_result[8], 4, &cp);
        b64_from_24bit(alt_result[9], alt_result[19], alt_result[29], 4, &cp);
        b64_from_24bit(0, alt_result[31], alt_result[30], 3, &cp);
        *cp = '\0';

        EVP_MD_CTX_free(ctx);
        EVP_MD_CTX_free(alt_ctx);
        return (0);

fail:
        if (ctx)
                EVP_MD_CTX_free(ctx);
        if (alt_ctx)
                EVP_MD_CTX_free(alt_ctx);
        return (-1);
}

/*
 * Implementation of the SHA-512 based password hash.  The salt may
 * include a custom rounds specification.  The resulting string is
 * stored in 'buffer'.
 */
static int
crypt_sha512(const char *key, const char *salt, char *buffer)
{
        unsigned long srounds;
        uint8_t alt_result[64], temp_result[64];
        EVP_MD_CTX *ctx, *alt_ctx;
        size_t salt_len, key_len, cnt, rounds;
        char *cp, *p_bytes, *s_bytes, *endp;
        const char *num;
        int rounds_custom = 0;
        static const char prefix[] = "$6$";
        static const char rounds_prefix[] = "rounds=";

        rounds = 5000;

        if (strncmp(prefix, salt, sizeof(prefix) - 1) == 0)
                salt += sizeof(prefix) - 1;

        if (strncmp(salt, rounds_prefix, sizeof(rounds_prefix) - 1) == 0) {
                num = salt + sizeof(rounds_prefix) - 1;
                srounds = strtoul(num, &endp, 10);
                if (*endp == '$') {
                        salt = endp + 1;
                        if (srounds < 1000)
                                srounds = 1000;
                        else if (srounds > 999999999)
                                srounds = 999999999;
                        rounds = srounds;
                        rounds_custom = 1;
                }
        }

        salt_len = strcspn(salt, "$");
        if (salt_len > 16)
                salt_len = 16;
        key_len = strlen(key);

        if (!OPENSSL_init_crypto(0, NULL))
                return (-1);

        ctx = EVP_MD_CTX_new();
        alt_ctx = EVP_MD_CTX_new();
        if (ctx == NULL || alt_ctx == NULL)
                goto fail;

        if (EVP_DigestInit_ex(ctx, EVP_sha512(), NULL) != 1)
                goto fail;
        EVP_DigestUpdate(ctx, key, key_len);
        EVP_DigestUpdate(ctx, salt, salt_len);

        if (EVP_DigestInit_ex(alt_ctx, EVP_sha512(), NULL) != 1)
                goto fail;
        EVP_DigestUpdate(alt_ctx, key, key_len);
        EVP_DigestUpdate(alt_ctx, salt, salt_len);
        EVP_DigestUpdate(alt_ctx, key, key_len);
        EVP_DigestFinal_ex(alt_ctx, alt_result, NULL);

        for (cnt = key_len; cnt > 64; cnt -= 64)
                EVP_DigestUpdate(ctx, alt_result, 64);
        EVP_DigestUpdate(ctx, alt_result, cnt);

        for (cnt = key_len; cnt > 0; cnt >>= 1)
                if (cnt & 1)
                        EVP_DigestUpdate(ctx, alt_result, 64);
                else
                        EVP_DigestUpdate(ctx, key, key_len);

        EVP_DigestFinal_ex(ctx, alt_result, NULL);

        EVP_DigestInit_ex(alt_ctx, EVP_sha512(), NULL);
        for (cnt = 0; cnt < key_len; ++cnt)
                EVP_DigestUpdate(alt_ctx, key, key_len);
        EVP_DigestFinal_ex(alt_ctx, temp_result, NULL);

        cp = p_bytes = alloca(key_len);
        for (cnt = key_len; cnt >= 64; cnt -= 64) {
                memcpy(cp, temp_result, 64);
                cp += 64;
        }
        memcpy(cp, temp_result, cnt);

        EVP_DigestInit_ex(alt_ctx, EVP_sha512(), NULL);
        for (cnt = 0; cnt < (size_t)(16 + alt_result[0]); ++cnt)
                EVP_DigestUpdate(alt_ctx, salt, salt_len);
        EVP_DigestFinal_ex(alt_ctx, temp_result, NULL);

        cp = s_bytes = alloca(salt_len);
        for (cnt = salt_len; cnt >= 64; cnt -= 64) {
                memcpy(cp, temp_result, 64);
                cp += 64;
        }
        memcpy(cp, temp_result, cnt);

        for (cnt = 0; cnt < rounds; ++cnt) {
                EVP_DigestInit_ex(ctx, EVP_sha512(), NULL);
                if (cnt & 1)
                        EVP_DigestUpdate(ctx, p_bytes, key_len);
                else
                        EVP_DigestUpdate(ctx, alt_result, 64);
                if (cnt % 3)
                        EVP_DigestUpdate(ctx, s_bytes, salt_len);
                if (cnt % 7)
                        EVP_DigestUpdate(ctx, p_bytes, key_len);
                if (cnt & 1)
                        EVP_DigestUpdate(ctx, alt_result, 64);
                else
                        EVP_DigestUpdate(ctx, p_bytes, key_len);
                EVP_DigestFinal_ex(ctx, alt_result, NULL);
        }

        cp = stpcpy(buffer, prefix);
        if (rounds_custom)
                cp += sprintf(cp, "%s%zu$", rounds_prefix, rounds);
        cp = stpncpy(cp, salt, salt_len);
        *cp++ = '$';

        b64_from_24bit(alt_result[0], alt_result[21], alt_result[42], 4, &cp);
        b64_from_24bit(alt_result[22], alt_result[43], alt_result[1], 4, &cp);
        b64_from_24bit(alt_result[44], alt_result[2], alt_result[23], 4, &cp);
        b64_from_24bit(alt_result[3], alt_result[24], alt_result[45], 4, &cp);
        b64_from_24bit(alt_result[25], alt_result[46], alt_result[4], 4, &cp);
        b64_from_24bit(alt_result[47], alt_result[5], alt_result[26], 4, &cp);
        b64_from_24bit(alt_result[6], alt_result[27], alt_result[48], 4, &cp);
        b64_from_24bit(alt_result[28], alt_result[49], alt_result[7], 4, &cp);
        b64_from_24bit(alt_result[50], alt_result[8], alt_result[29], 4, &cp);
        b64_from_24bit(alt_result[9], alt_result[30], alt_result[51], 4, &cp);
        b64_from_24bit(alt_result[31], alt_result[52], alt_result[10], 4, &cp);
        b64_from_24bit(alt_result[53], alt_result[11], alt_result[32], 4, &cp);
        b64_from_24bit(alt_result[12], alt_result[33], alt_result[54], 4, &cp);
        b64_from_24bit(alt_result[34], alt_result[55], alt_result[13], 4, &cp);
        b64_from_24bit(alt_result[56], alt_result[14], alt_result[35], 4, &cp);
        b64_from_24bit(alt_result[15], alt_result[36], alt_result[57], 4, &cp);
        b64_from_24bit(alt_result[37], alt_result[58], alt_result[16], 4, &cp);
        b64_from_24bit(alt_result[59], alt_result[17], alt_result[38], 4, &cp);
        b64_from_24bit(alt_result[18], alt_result[39], alt_result[60], 4, &cp);
        b64_from_24bit(alt_result[40], alt_result[61], alt_result[19], 4, &cp);
        b64_from_24bit(alt_result[62], alt_result[20], alt_result[41], 4, &cp);
        b64_from_24bit(0, 0, alt_result[63], 2, &cp);
        *cp = '\0';

        EVP_MD_CTX_free(ctx);
        EVP_MD_CTX_free(alt_ctx);
        return (0);

fail:
        if (ctx)
                EVP_MD_CTX_free(ctx);
        if (alt_ctx)
                EVP_MD_CTX_free(alt_ctx);
        return (-1);
}

/*
 * Wrapper for vlibc: expose crypt() using the DES implementation above
 * and delegate to the host crypt() when stronger algorithms are
 * requested on BSD systems.
 */
extern char *host_crypt(const char *, const char *) __asm("crypt");

char *
/*
 * Dispatch wrapper that selects the appropriate hashing algorithm
 * based on the prefix of 'salt'.  Returns a pointer to a thread-local
 * buffer containing the hashed password.
 */
crypt(const char *key, const char *salt)
{
    static __thread char buf[128];
#if defined(__FreeBSD__) || defined(__NetBSD__) || \
    defined(__OpenBSD__) || defined(__DragonFly__)
    /* defer to the system crypt for algorithms we don't implement */
    if (salt && salt[0] == '$' && strncmp(salt, "$1$", 3) != 0 &&
        strncmp(salt, "$5$", 3) != 0 && strncmp(salt, "$6$", 3) != 0)
        return host_crypt(key, salt);
#endif
    if (salt && strncmp(salt, "$1$", 3) == 0) {
        if (crypt_md5(key, salt, buf) != 0)
#ifdef __GLIBC__
            return host_crypt(key, salt);
#else
            return NULL;
#endif
        return buf;
    }
    if (salt && strncmp(salt, "$5$", 3) == 0) {
        if (crypt_sha256(key, salt, buf) != 0)
#ifdef __GLIBC__
            return host_crypt(key, salt);
#else
            return NULL;
#endif
        return buf;
    }
    if (salt && strncmp(salt, "$6$", 3) == 0) {
        if (crypt_sha512(key, salt, buf) != 0)
#ifdef __GLIBC__
            return host_crypt(key, salt);
#else
            return NULL;
#endif
        return buf;
    }

    if (crypt_des(key, salt, buf) != 0)
#ifdef __GLIBC__
        return host_crypt(key, salt);
#else
        return NULL;
#endif
    return buf;
}
