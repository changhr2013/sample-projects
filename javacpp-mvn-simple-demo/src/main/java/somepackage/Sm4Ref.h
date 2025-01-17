// sm4_ref.c
// 2018-04-20  Markku-Juhani O. Saarinen <mjos@iki.fi>

// Reference implementation of SM4, the Chinese Encryption Standard.
// Adopted from Internet Draft draft-ribose-cfrg-sm4 with some modifications.

//#include "sm4_ref.h"

#ifndef SM4_REF_H
#define SM4_REF_H

#define SM4_BLOCK_SIZE    16
#define SM4_KEY_SCHEDULE  32

#include <stdint.h>

// sm4ni.c
// 2018-04-20  Markku-Juhani O. Saarinen <mjos@iki.fi>

// Vectorized implementation of SM4. Uses affine transformations and AES NI
// to implement the SM4 S-Box.

//#include "sm4_ref.h"
#include <x86intrin.h>

// Encrypt 4 blocks (64 bytes) in ECB mode

void sm4_encrypt4(const uint32_t rk[32], uint32_t *src, const uint32_t *dst)
{
    // nibble mask
    const __m128i c0f __attribute__((aligned(0x10))) =
        { 0x0F0F0F0F0F0F0F0F, 0x0F0F0F0F0F0F0F0F };

    // flip all bytes in all 32-bit words
    const __m128i flp __attribute__((aligned(0x10))) =
        { 0x0405060700010203, 0x0C0D0E0F08090A0B };

    // inverse shift rows
    const __m128i shr __attribute__((aligned(0x10))) =
        { 0x0B0E0104070A0D00, 0x0306090C0F020508 };

    // Affine transform 1 (low and high hibbles)
    const __m128i m1l __attribute__((aligned(0x10))) =
        { (long long int)0x9197E2E474720701, (long long int)0xC7C1B4B222245157 };
    const __m128i m1h __attribute__((aligned(0x10))) =
        { (long long int)0xE240AB09EB49A200, (long long int)0xF052B91BF95BB012 };

    // Affine transform 2 (low and high hibbles)
    const __m128i m2l __attribute__((aligned(0x10))) =
        { (long long int)0x5B67F2CEA19D0834, (long long int)0xEDD14478172BBE82 };
    const __m128i m2h __attribute__((aligned(0x10))) =
        { (long long int)0xAE7201DD73AFDC00, (long long int)0x11CDBE62CC1063BF };

    // left rotations of 32-bit words by 8-bit increments
    const __m128i r08 __attribute__((aligned(0x10))) =
        { 0x0605040702010003, 0x0E0D0C0F0A09080B };
    const __m128i r16 __attribute__((aligned(0x10))) =
        { 0x0504070601000302, 0x0D0C0F0E09080B0A };
    const __m128i r24 __attribute__((aligned(0x10))) =
        { 0x0407060500030201, 0x0C0F0E0D080B0A09 };

    __m128i x, y, t0, t1, t2, t3;

    uint32_t k, *p32, v[4] __attribute__((aligned(0x10)));
    int i;

    p32 = (uint32_t *) src;
    t0 = _mm_set_epi32(p32[12], p32[ 8], p32[ 4], p32[ 0]);
    t0 = _mm_shuffle_epi8(t0, flp);
    t1 = _mm_set_epi32(p32[13], p32[ 9], p32[ 5], p32[ 1]);
    t1 = _mm_shuffle_epi8(t1, flp);
    t2 = _mm_set_epi32(p32[14], p32[10], p32[ 6], p32[ 2]);
    t2 = _mm_shuffle_epi8(t2, flp);
    t3 = _mm_set_epi32(p32[15], p32[11], p32[ 7], p32[ 3]);
    t3 = _mm_shuffle_epi8(t3, flp);

#ifndef SM4NI_UNROLL

    // not unrolled

    for (i = 0; i < 32; i++) {

        k = rk[i];
        x = t1 ^ t2 ^ t3 ^ _mm_set_epi32(k, k, k, k);

        y = _mm_and_si128(x, c0f);          // inner affine
        y = _mm_shuffle_epi8(m1l, y);
        x = _mm_srli_epi64(x, 4);
        x = _mm_and_si128(x, c0f);
        x = _mm_shuffle_epi8(m1h, x) ^ y;

        x = _mm_shuffle_epi8(x, shr);       // inverse MixColumns
        x = _mm_aesenclast_si128(x, c0f);   // AESNI instruction

        y = _mm_andnot_si128(x, c0f);       // outer affine
        y = _mm_shuffle_epi8(m2l, y);
        x = _mm_srli_epi64(x, 4);
        x = _mm_and_si128(x, c0f);
        x = _mm_shuffle_epi8(m2h, x) ^ y;

        // 4 parallel L1 linear transforms
        y = x ^ _mm_shuffle_epi8(x, r08) ^ _mm_shuffle_epi8(x, r16);
        y = _mm_slli_epi32(y, 2) ^ _mm_srli_epi32(y, 30);
        x = x ^ y ^ _mm_shuffle_epi8(x, r24);

        // rotate registers
        x ^= t0;
        t0 = t1;
        t1 = t2;
        t2 = t3;
        t3 = x;
    }

#else

    // unrolled version

#define SM4_TAU_L1 { \
    y = _mm_and_si128(x, c0f);              \
    y = _mm_shuffle_epi8(m1l, y);           \
    x = _mm_srli_epi64(x, 4);               \
    x = _mm_and_si128(x, c0f);              \
    x = _mm_shuffle_epi8(m1h, x) ^ y;       \
    x = _mm_shuffle_epi8(x, shr);           \
    x = _mm_aesenclast_si128(x, c0f);       \
    y = _mm_andnot_si128(x, c0f);           \
    y = _mm_shuffle_epi8(m2l, y);           \
    x = _mm_srli_epi64(x, 4);               \
    x = _mm_and_si128(x, c0f);              \
    x = _mm_shuffle_epi8(m2h, x) ^ y;       \
    y = x ^ _mm_shuffle_epi8(x, r08) ^      \
        _mm_shuffle_epi8(x, r16);           \
    y = _mm_slli_epi32(y, 2) ^              \
        _mm_srli_epi32(y, 30);              \
    x = x ^ y ^ _mm_shuffle_epi8(x, r24);   \
}

    for (i = 0; i < 32;) {

        k = rk[i++];
        x = t1 ^ t2 ^ t3 ^ _mm_set_epi32(k, k, k, k);
        SM4_TAU_L1
        t0 ^= x;

        k = rk[i++];
        x = t0 ^ t2 ^ t3 ^ _mm_set_epi32(k, k, k, k);
        SM4_TAU_L1
        t1 ^= x;

        k = rk[i++];
        x = t0 ^ t1 ^ t3 ^ _mm_set_epi32(k, k, k, k);
        SM4_TAU_L1
        t2 ^= x;

        k = rk[i++];
        x = t0 ^ t1 ^ t2 ^ _mm_set_epi32(k, k, k, k);
        SM4_TAU_L1
        t3 ^= x;
    }

#endif

    p32 = (uint32_t *) dst;

    _mm_store_si128((__m128i *) v, _mm_shuffle_epi8(t3, flp));
    p32[ 0] = v[0];
    p32[ 4] = v[1];
    p32[ 8] = v[2];
    p32[12] = v[3];

    _mm_store_si128((__m128i *) v, _mm_shuffle_epi8(t2, flp));
    p32[ 1] = v[0];
    p32[ 5] = v[1];
    p32[ 9] = v[2];
    p32[13] = v[3];

    _mm_store_si128((__m128i *) v, _mm_shuffle_epi8(t1, flp));
    p32[ 2] = v[0];
    p32[ 6] = v[1];
    p32[10] = v[2];
    p32[14] = v[3];

    _mm_store_si128((__m128i *) v, _mm_shuffle_epi8(t0, flp));
    p32[ 3] = v[0];
    p32[ 7] = v[1];
    p32[11] = v[2];
    p32[15] = v[3];
}


/* Operations */
/* Rotate Left 32-bit number */
#define ROTL32(x, n) (((x) << (n)) | ((x) >> (32 - (n))))

static const uint32_t sm4_ck[32] = {
    0x00070E15, 0x1C232A31, 0x383F464D, 0x545B6269,
    0x70777E85, 0x8C939AA1, 0xA8AFB6BD, 0xC4CBD2D9,
    0xE0E7EEF5, 0xFC030A11, 0x181F262D, 0x343B4249,
    0x50575E65, 0x6C737A81, 0x888F969D, 0xA4ABB2B9,
    0xC0C7CED5, 0xDCE3EAF1, 0xF8FF060D, 0x141B2229,
    0x30373E45, 0x4C535A61, 0x686F767D, 0x848B9299,
    0xA0A7AEB5, 0xBCC3CAD1, 0xD8DFE6ED, 0xF4FB0209,
    0x10171E25, 0x2C333A41, 0x484F565D, 0x646B7279
};

static const uint8_t sm4_sbox[256] = {
    0xD6, 0x90, 0xE9, 0xFE, 0xCC, 0xE1, 0x3D, 0xB7,
    0x16, 0xB6, 0x14, 0xC2, 0x28, 0xFB, 0x2C, 0x05,
    0x2B, 0x67, 0x9A, 0x76, 0x2A, 0xBE, 0x04, 0xC3,
    0xAA, 0x44, 0x13, 0x26, 0x49, 0x86, 0x06, 0x99,
    0x9C, 0x42, 0x50, 0xF4, 0x91, 0xEF, 0x98, 0x7A,
    0x33, 0x54, 0x0B, 0x43, 0xED, 0xCF, 0xAC, 0x62,
    0xE4, 0xB3, 0x1C, 0xA9, 0xC9, 0x08, 0xE8, 0x95,
    0x80, 0xDF, 0x94, 0xFA, 0x75, 0x8F, 0x3F, 0xA6,
    0x47, 0x07, 0xA7, 0xFC, 0xF3, 0x73, 0x17, 0xBA,
    0x83, 0x59, 0x3C, 0x19, 0xE6, 0x85, 0x4F, 0xA8,
    0x68, 0x6B, 0x81, 0xB2, 0x71, 0x64, 0xDA, 0x8B,
    0xF8, 0xEB, 0x0F, 0x4B, 0x70, 0x56, 0x9D, 0x35,
    0x1E, 0x24, 0x0E, 0x5E, 0x63, 0x58, 0xD1, 0xA2,
    0x25, 0x22, 0x7C, 0x3B, 0x01, 0x21, 0x78, 0x87,
    0xD4, 0x00, 0x46, 0x57, 0x9F, 0xD3, 0x27, 0x52,
    0x4C, 0x36, 0x02, 0xE7, 0xA0, 0xC4, 0xC8, 0x9E,
    0xEA, 0xBF, 0x8A, 0xD2, 0x40, 0xC7, 0x38, 0xB5,
    0xA3, 0xF7, 0xF2, 0xCE, 0xF9, 0x61, 0x15, 0xA1,
    0xE0, 0xAE, 0x5D, 0xA4, 0x9B, 0x34, 0x1A, 0x55,
    0xAD, 0x93, 0x32, 0x30, 0xF5, 0x8C, 0xB1, 0xE3,
    0x1D, 0xF6, 0xE2, 0x2E, 0x82, 0x66, 0xCA, 0x60,
    0xC0, 0x29, 0x23, 0xAB, 0x0D, 0x53, 0x4E, 0x6F,
    0xD5, 0xDB, 0x37, 0x45, 0xDE, 0xFD, 0x8E, 0x2F,
    0x03, 0xFF, 0x6A, 0x72, 0x6D, 0x6C, 0x5B, 0x51,
    0x8D, 0x1B, 0xAF, 0x92, 0xBB, 0xDD, 0xBC, 0x7F,
    0x11, 0xD9, 0x5C, 0x41, 0x1F, 0x10, 0x5A, 0xD8,
    0x0A, 0xC1, 0x31, 0x88, 0xA5, 0xCD, 0x7B, 0xBD,
    0x2D, 0x74, 0xD0, 0x12, 0xB8, 0xE5, 0xB4, 0xB0,
    0x89, 0x69, 0x97, 0x4A, 0x0C, 0x96, 0x77, 0x7E,
    0x65, 0xB9, 0xF1, 0x09, 0xC5, 0x6E, 0xC6, 0x84,
    0x18, 0xF0, 0x7D, 0xEC, 0x3A, 0xDC, 0x4D, 0x20,
    0x79, 0xEE, 0x5F, 0x3E, 0xD7, 0xCB, 0x39, 0x48
};

static const uint32_t sm4_fk[4] = {
  0xA3B1BAC6, 0x56AA3350, 0x677D9197, 0xB27022DC
};

static uint32_t load_u32_be(const uint8_t *b, uint32_t n)
{
  return ((uint32_t)b[4 * n    ] << 24) |
         ((uint32_t)b[4 * n + 1] << 16) |
         ((uint32_t)b[4 * n + 2] << 8)  |
         ((uint32_t)b[4 * n + 3]);
}

static void store_u32_be(uint32_t v, uint8_t *b)
{
  b[0] = (uint8_t)(v >> 24);
  b[1] = (uint8_t)(v >> 16);
  b[2] = (uint8_t)(v >> 8);
  b[3] = (uint8_t)(v);
}

void sm4_key_schedule(const uint8_t key[], uint32_t rk[])
{
  uint32_t t, x, k[36];
  int i;

  for (i = 0; i < 4; i++)
  {
    k[i] = load_u32_be(key, i) ^ sm4_fk[i];
  }

  /* T' */
  for (i = 0; i < SM4_KEY_SCHEDULE; ++i)
  {
    x = k[i + 1] ^ k[i + 2] ^ k[i + 3] ^ sm4_ck[i];

    /* Nonlinear operation tau */
    t = ((uint32_t)sm4_sbox[(uint8_t)(x >> 24)]) << 24 |
        ((uint32_t)sm4_sbox[(uint8_t)(x >> 16)]) << 16 |
        ((uint32_t)sm4_sbox[(uint8_t)(x >>  8)]) <<  8 |
        ((uint32_t)sm4_sbox[(uint8_t)(x)]);

    /* Linear operation L' */
    k[i+4] = k[i] ^ (t ^ ROTL32(t, 13) ^ ROTL32(t, 23));
    rk[i] = k[i + 4];
  }
}

#define SM4_ROUNDS(k0, k1, k2, k3, F)   \
  do {                                  \
    x0 ^= F(x1 ^ x2 ^ x3 ^ rk[k0]); \
    x1 ^= F(x0 ^ x2 ^ x3 ^ rk[k1]); \
    x2 ^= F(x0 ^ x1 ^ x3 ^ rk[k2]); \
    x3 ^= F(x0 ^ x1 ^ x2 ^ rk[k3]); \
  } while(0)

static uint32_t sm4_t(uint32_t x)
{
  uint32_t t = 0;

  t |= ((uint32_t)sm4_sbox[(uint8_t)(x >> 24)]) << 24;
  t |= ((uint32_t)sm4_sbox[(uint8_t)(x >> 16)]) << 16;
  t |= ((uint32_t)sm4_sbox[(uint8_t)(x >> 8)]) << 8;
  t |= sm4_sbox[(uint8_t)x];

  /*
   * L linear transform
   */
  return t ^ ROTL32(t, 2) ^ ROTL32(t, 10) ^
         ROTL32(t, 18) ^ ROTL32(t, 24);
}

void sm4_encrypt(const uint32_t rk[SM4_KEY_SCHEDULE],
    const uint8_t *plaintext, uint8_t *ciphertext)
{
  uint32_t x0, x1, x2, x3;

  x0 = load_u32_be(plaintext, 0);
  x1 = load_u32_be(plaintext, 1);
  x2 = load_u32_be(plaintext, 2);
  x3 = load_u32_be(plaintext, 3);

  SM4_ROUNDS( 0,  1,  2,  3, sm4_t);
  SM4_ROUNDS( 4,  5,  6,  7, sm4_t);
  SM4_ROUNDS( 8,  9, 10, 11, sm4_t);
  SM4_ROUNDS(12, 13, 14, 15, sm4_t);
  SM4_ROUNDS(16, 17, 18, 19, sm4_t);
  SM4_ROUNDS(20, 21, 22, 23, sm4_t);
  SM4_ROUNDS(24, 25, 26, 27, sm4_t);
  SM4_ROUNDS(28, 29, 30, 31, sm4_t);

  store_u32_be(x3, ciphertext);
  store_u32_be(x2, ciphertext + 4);
  store_u32_be(x1, ciphertext + 8);
  store_u32_be(x0, ciphertext + 12);
}

void sm4_decrypt(const uint32_t rk[SM4_KEY_SCHEDULE],
    const uint8_t *ciphertext, uint8_t *plaintext)
{
  uint32_t x0, x1, x2, x3;

  x0 = load_u32_be(ciphertext, 0);
  x1 = load_u32_be(ciphertext, 1);
  x2 = load_u32_be(ciphertext, 2);
  x3 = load_u32_be(ciphertext, 3);

  SM4_ROUNDS(31, 30, 29, 28, sm4_t);
  SM4_ROUNDS(27, 26, 25, 24, sm4_t);
  SM4_ROUNDS(23, 22, 21, 20, sm4_t);
  SM4_ROUNDS(19, 18, 17, 16, sm4_t);
  SM4_ROUNDS(15, 14, 13, 12, sm4_t);
  SM4_ROUNDS(11, 10,  9,  8, sm4_t);
  SM4_ROUNDS( 7,  6,  5,  4, sm4_t);
  SM4_ROUNDS( 3,  2,  1,  0, sm4_t);

  store_u32_be(x3, plaintext);
  store_u32_be(x2, plaintext + 4);
  store_u32_be(x1, plaintext + 8);
  store_u32_be(x0, plaintext + 12);
}

int add(int a, int b, int c) {
    return a + b + c;
}

#endif

