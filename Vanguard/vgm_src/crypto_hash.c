/*
 * crypto_hash.c - Cryptographic & Hashing Suite in .riot1
 * Reconstructed via REA from vgm.exe:
 *   - FUN_1419af5b3 @ 0x1419af5b3 : SHA-1 (FIPS 180-4)
 *   - FUN_1419b4277 @ 0x1419b4277 : SHA-256 (FIPS 180-4)
 *   - FUN_1419c95bc @ 0x1419c95bc : SHA-512 (FIPS 180-4)
 *   - FUN_1419be9d2 @ 0x1419be9d2 : XXH64 (primes 0x9E3779B185EBCA87, 0xC2B2AE3D27D4EB4F)
 *   - FUN_1419cd013 @ 0x1419cd013 : XXH3-128 / 128-bit non-cryptographic digest
 *   - FUN_1419c3d8d @ 0x1419c3d8d : Custom wide formatter supporting %Z / %wZ (UNICODE_STRING)
 */

#include "vgm.h"
#include <stdarg.h>

/* ============================================================================
 * 1. SHA-1 (FUN_1419af5b3)
 * ============================================================================ */

static void VgmSha1Transform(uint32_t State[5], const uint8_t Block[64])
{
    uint32_t W[80];
    for (int i = 0; i < 16; ++i) {
        W[i] = ((uint32_t)Block[i * 4 + 0] << 24) |
               ((uint32_t)Block[i * 4 + 1] << 16) |
               ((uint32_t)Block[i * 4 + 2] <<  8) |
               ((uint32_t)Block[i * 4 + 3]);
    }
    for (int i = 16; i < 80; ++i) {
        W[i] = _rotl(W[i - 3] ^ W[i - 8] ^ W[i - 14] ^ W[i - 16], 1);
    }

    uint32_t a = State[0];
    uint32_t b = State[1];
    uint32_t c = State[2];
    uint32_t d = State[3];
    uint32_t e = State[4];

    for (int i = 0; i < 80; ++i) {
        uint32_t f, k;
        if (i < 20) {
            f = (b & c) | ((~b) & d);
            k = 0x5A827999U;
        } else if (i < 40) {
            f = b ^ c ^ d;
            k = 0x6ED9EBA1U;
        } else if (i < 60) {
            f = (b & c) | (b & d) | (c & d);
            k = 0x8F1BBCDCU;
        } else {
            f = b ^ c ^ d;
            k = 0xCA62C1D6U;
        }
        uint32_t temp = _rotl(a, 5) + f + e + k + W[i];
        e = d;
        d = c;
        c = _rotl(b, 30);
        b = a;
        a = temp;
    }

    State[0] += a;
    State[1] += b;
    State[2] += c;
    State[3] += d;
    State[4] += e;
}

void VgmSha1Compute(const void *Data, size_t Len, uint8_t Digest[20])
{
    uint32_t state[5] = {
        0x67452301U, 0xEFCDAB89U, 0x98BADCFEU, 0x10325476U, 0xC3D2E1F0U
    };
    const uint8_t *bytes = (const uint8_t *)Data;
    uint64_t bitLen = (uint64_t)Len * 8ULL;

    while (Len >= 64) {
        VgmSha1Transform(state, bytes);
        bytes += 64;
        Len -= 64;
    }

    uint8_t tail[128];
    ZeroMemory(tail, sizeof(tail));
    if (Len > 0) {
        memcpy(tail, bytes, Len);
    }
    tail[Len] = 0x80;

    size_t totalBlockBytes = (Len < 56) ? 64 : 128;
    for (int i = 0; i < 8; ++i) {
        tail[totalBlockBytes - 1 - i] = (uint8_t)(bitLen >> (i * 8));
    }

    VgmSha1Transform(state, tail);
    if (totalBlockBytes == 128) {
        VgmSha1Transform(state, tail + 64);
    }

    for (int i = 0; i < 5; ++i) {
        Digest[i * 4 + 0] = (uint8_t)(state[i] >> 24);
        Digest[i * 4 + 1] = (uint8_t)(state[i] >> 16);
        Digest[i * 4 + 2] = (uint8_t)(state[i] >>  8);
        Digest[i * 4 + 3] = (uint8_t)(state[i]);
    }
}

/* ============================================================================
 * 2. SHA-256 (FUN_1419b4277)
 * ============================================================================ */

static const uint32_t g_Sha256K[64] = {
    0x428A2F98U, 0x71374491U, 0xB5C0FBCFU, 0xE9B5DBA5U, 0x3956C25BU, 0x59F111F1U, 0x923F82A4U, 0xAB1C5ED5U,
    0xD807AA98U, 0x12835B01U, 0x243185BEU, 0x550C7DC3U, 0x72BE5D74U, 0x80DEB1FEU, 0x9BDC06A7U, 0xC19BF174U,
    0xE49B69C1U, 0xEFBE4786U, 0x0FC19DC6U, 0x240CA1CCU, 0x2DE92C6FU, 0x4A7484AAU, 0x5CB0A9DCU, 0x76F988DAU,
    0x983E5152U, 0xA831C66DU, 0xB00327C8U, 0xBF597FC7U, 0xC6E00BF3U, 0xD5A79147U, 0x06CA6351U, 0x14292967U,
    0x27B70A85U, 0x2E1B2138U, 0x4D2C6DFCU, 0x53380D13U, 0x650A7354U, 0x766A0ABBU, 0x81C2C92EU, 0x92722C85U,
    0xA2BFE8A1U, 0xA81A664BU, 0xC24B8B70U, 0xC76C51A3U, 0xD192E819U, 0xD6990624U, 0xF40E3585U, 0x106AA070U,
    0x19A4C116U, 0x1E376C08U, 0x2748774CU, 0x34B0BCB5U, 0x391C0CB3U, 0x4ED8AA4AU, 0x5B9CCA4FU, 0x682E6FF3U,
    0x748F82EEU, 0x78A5636FU, 0x84C87814U, 0x8CC70208U, 0x90BEFFFAU, 0xA4506CEBU, 0xBEF9A3F7U, 0xC67178F2U
};

static void VgmSha256Transform(uint32_t State[8], const uint8_t Block[64])
{
    uint32_t W[64];
    for (int i = 0; i < 16; ++i) {
        W[i] = ((uint32_t)Block[i * 4 + 0] << 24) |
               ((uint32_t)Block[i * 4 + 1] << 16) |
               ((uint32_t)Block[i * 4 + 2] <<  8) |
               ((uint32_t)Block[i * 4 + 3]);
    }
    for (int i = 16; i < 64; ++i) {
        uint32_t s0 = _rotr(W[i - 15], 7) ^ _rotr(W[i - 15], 18) ^ (W[i - 15] >> 3);
        uint32_t s1 = _rotr(W[i - 2], 17) ^ _rotr(W[i - 2], 19) ^ (W[i - 2] >> 10);
        W[i] = W[i - 16] + s0 + W[i - 7] + s1;
    }

    uint32_t a = State[0], b = State[1], c = State[2], d = State[3];
    uint32_t e = State[4], f = State[5], g = State[6], h = State[7];

    for (int i = 0; i < 64; ++i) {
        uint32_t S1 = _rotr(e, 6) ^ _rotr(e, 11) ^ _rotr(e, 25);
        uint32_t ch = (e & f) ^ ((~e) & g);
        uint32_t temp1 = h + S1 + ch + g_Sha256K[i] + W[i];
        uint32_t S0 = _rotr(a, 2) ^ _rotr(a, 13) ^ _rotr(a, 22);
        uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
        uint32_t temp2 = S0 + maj;

        h = g;
        g = f;
        f = e;
        e = d + temp1;
        d = c;
        c = b;
        b = a;
        a = temp1 + temp2;
    }

    State[0] += a; State[1] += b; State[2] += c; State[3] += d;
    State[4] += e; State[5] += f; State[6] += g; State[7] += h;
}

void VgmSha256Compute(const void *Data, size_t Len, uint8_t Digest[32])
{
    uint32_t state[8] = {
        0x6A09E667U, 0xBB67AE85U, 0x3C6EF372U, 0xA54FF53AU,
        0x510E527FU, 0x9B05688CU, 0x1F83D9ABU, 0x5BE0CD19U
    };
    const uint8_t *bytes = (const uint8_t *)Data;
    uint64_t bitLen = (uint64_t)Len * 8ULL;

    while (Len >= 64) {
        VgmSha256Transform(state, bytes);
        bytes += 64;
        Len -= 64;
    }

    uint8_t tail[128];
    ZeroMemory(tail, sizeof(tail));
    if (Len > 0) {
        memcpy(tail, bytes, Len);
    }
    tail[Len] = 0x80;

    size_t totalBlockBytes = (Len < 56) ? 64 : 128;
    for (int i = 0; i < 8; ++i) {
        tail[totalBlockBytes - 1 - i] = (uint8_t)(bitLen >> (i * 8));
    }

    VgmSha256Transform(state, tail);
    if (totalBlockBytes == 128) {
        VgmSha256Transform(state, tail + 64);
    }

    for (int i = 0; i < 8; ++i) {
        Digest[i * 4 + 0] = (uint8_t)(state[i] >> 24);
        Digest[i * 4 + 1] = (uint8_t)(state[i] >> 16);
        Digest[i * 4 + 2] = (uint8_t)(state[i] >>  8);
        Digest[i * 4 + 3] = (uint8_t)(state[i]);
    }
}

/* ============================================================================
 * 3. SHA-512 (FUN_1419c95bc)
 * ============================================================================ */

static const uint64_t g_Sha512K[80] = {
    0x428A2F98D728AE22ULL, 0x7137449123EF65CDULL, 0xB5C0FBCFEC4D3B2FULL, 0xE9B5DBA58189DBBCULL,
    0x3956C25BF348B538ULL, 0x59F111F1B605D019ULL, 0x923F82A4AF194F9BULL, 0xAB1C5ED5DA6D8118ULL,
    0xD807AA98A3030242ULL, 0x12835B014EE4B28CULL, 0x243185BE4EE4B28CULL, 0x550C7DC3D5FFB4E2ULL,
    0x72BE5D74F27B896FULL, 0x80DEB1FE3B1696B1ULL, 0x9BDC06A725C71235ULL, 0xC19BF174CF692694ULL,
    0xE49B69C19EF14AD2ULL, 0xEFBE4786384F25E3ULL, 0x0FC19DC68B8CD5B5ULL, 0x240CA1CC77AC9C65ULL,
    0x2DE92C6F592B0275ULL, 0x4A7484AA6EA6E483ULL, 0x5CB0A9DCBD41FBD4ULL, 0x76F988DA831153B5ULL,
    0x983E5152EE66DFABULL, 0xA831C66D2DB43210ULL, 0xB00327C898FB213FULL, 0xBF597FC7BEEF0EE4ULL,
    0xC6E00BF33DA88FC2ULL, 0xD5A79147930AA725ULL, 0x06CA6351E003826FULL, 0x142929670A0E6E70ULL,
    0x27B70A8546D22FFCULL, 0x2E1B21385C26C926ULL, 0x4D2C6DFC5AC42AEDULL, 0x53380D139D95B3DFULL,
    0x650A73548BAF63DEULL, 0x766A0ABB3C77B2A8ULL, 0x81C2C92E47EDAEE6ULL, 0x92722C851482353BULL,
    0xA2BFE8A14CF10364ULL, 0xA81A664BBC423001ULL, 0xC24B8B70D0F89791ULL, 0xC76C51A30654BE30ULL,
    0xD192E819D6EF5218ULL, 0xD69906245565A910ULL, 0xF40E35855771202AULL, 0x106AA07032BBD1B8ULL,
    0x19A4C116B8D2D0C8ULL, 0x1E376C085141AB53ULL, 0x2748774CDF8EEB99ULL, 0x34B0BCB5E19B48A8ULL,
    0x391C0CB3C5C95A63ULL, 0x4ED8AA4AE3418ACBULL, 0x5B9CCA4F7763E373ULL, 0x682E6FF3D6B2B8A3ULL,
    0x748F82EE5DEFB2FCULL, 0x78A5636F43172F60ULL, 0x84C87814A1F0AB72ULL, 0x8CC702081A6439ECULL,
    0x90BEFFFA23631E28ULL, 0xA4506CEBDE82BDE9ULL, 0xBEF9A3F7B2C67915ULL, 0xC67178F2E372532BULL,
    0xCA273ECEEA26619CULL, 0xD186B8C721C0C207ULL, 0xEADA7DD6CDE0EB1EULL, 0xF57D4F7FEE6ED178ULL,
    0x06F067AA72176FBAULL, 0x0A637DC5A2C898A6ULL, 0x113F9804BEF90DAEULL, 0x1B710B35131C471BULL,
    0x28DB77F523047D84ULL, 0x32CAAB7B40C72493ULL, 0x3C9EBE0A15C9BEBCULL, 0x431D67C49C100D4CULL,
    0x4CC5D4BECB3E42B6ULL, 0x597F299CFC657E2AULL, 0x5FCB6FAB3AD6FAECULL, 0x6C44198C4A475817ULL
};

static void VgmSha512Transform(uint64_t State[8], const uint8_t Block[128])
{
    uint64_t W[80];
    for (int i = 0; i < 16; ++i) {
        uint64_t val = 0;
        for (int b = 0; b < 8; ++b) {
            val = (val << 8) | Block[i * 8 + b];
        }
        W[i] = val;
    }
    for (int i = 16; i < 80; ++i) {
        uint64_t s0 = _rotr64(W[i - 15], 1) ^ _rotr64(W[i - 15], 8) ^ (W[i - 15] >> 7);
        uint64_t s1 = _rotr64(W[i - 2], 19) ^ _rotr64(W[i - 2], 61) ^ (W[i - 2] >> 6);
        W[i] = W[i - 16] + s0 + W[i - 7] + s1;
    }

    uint64_t a = State[0], b = State[1], c = State[2], d = State[3];
    uint64_t e = State[4], f = State[5], g = State[6], h = State[7];

    for (int i = 0; i < 80; ++i) {
        uint64_t S1 = _rotr64(e, 14) ^ _rotr64(e, 18) ^ _rotr64(e, 41);
        uint64_t ch = (e & f) ^ ((~e) & g);
        uint64_t temp1 = h + S1 + ch + g_Sha512K[i] + W[i];
        uint64_t S0 = _rotr64(a, 28) ^ _rotr64(a, 34) ^ _rotr64(a, 39);
        uint64_t maj = (a & b) ^ (a & c) ^ (b & c);
        uint64_t temp2 = S0 + maj;

        h = g; g = f; f = e; e = d + temp1;
        d = c; c = b; b = a; a = temp1 + temp2;
    }

    State[0] += a; State[1] += b; State[2] += c; State[3] += d;
    State[4] += e; State[5] += f; State[6] += g; State[7] += h;
}

void VgmSha512Compute(const void *Data, size_t Len, uint8_t Digest[64])
{
    uint64_t state[8] = {
        0x6A09E667F3BCC908ULL, 0xBB67AE8584CAA73BULL,
        0x3C6EF372FE94F82BULL, 0xA54FF53A5F1D36F1ULL,
        0x510E527FADE682D1ULL, 0x9B05688C2B3E6C1FULL,
        0x1F83D9ABFB41BD6BULL, 0x5BE0CD19137E2179ULL
    };
    const uint8_t *bytes = (const uint8_t *)Data;
    uint64_t bitLen = (uint64_t)Len * 8ULL;

    while (Len >= 128) {
        VgmSha512Transform(state, bytes);
        bytes += 128;
        Len -= 128;
    }

    uint8_t tail[256];
    ZeroMemory(tail, sizeof(tail));
    if (Len > 0) {
        memcpy(tail, bytes, Len);
    }
    tail[Len] = 0x80;

    size_t totalBlockBytes = (Len < 112) ? 128 : 256;
    for (int i = 0; i < 8; ++i) {
        tail[totalBlockBytes - 1 - i] = (uint8_t)(bitLen >> (i * 8));
    }

    VgmSha512Transform(state, tail);
    if (totalBlockBytes == 256) {
        VgmSha512Transform(state, tail + 128);
    }

    for (int i = 0; i < 8; ++i) {
        for (int b = 0; b < 8; ++b) {
            Digest[i * 8 + b] = (uint8_t)(state[i] >> ((7 - b) * 8));
        }
    }
}

/* ============================================================================
 * 4. XXH64 & XXH3-128 (FUN_1419be9d2 & FUN_1419cd013)
 * ============================================================================ */

#define XXH_PRIME64_1   0x9E3779B185EBCA87ULL
#define XXH_PRIME64_2   0xC2B2AE3D27D4EB4FULL
#define XXH_PRIME64_3   0x165667B19E3779F9ULL
#define XXH_PRIME64_4   0x85EBCA77C2B2AE63ULL
#define XXH_PRIME64_5   0x27D4EB2F165667C5ULL

static uint64_t VgmXxh64Round(uint64_t Acc, uint64_t Input)
{
    Acc += Input * XXH_PRIME64_2;
    Acc  = _rotl64(Acc, 31);
    Acc *= XXH_PRIME64_1;
    return Acc;
}

static uint64_t VgmXxh64Avalanche(uint64_t Hash)
{
    Hash ^= Hash >> 33;
    Hash *= XXH_PRIME64_2;
    Hash ^= Hash >> 29;
    Hash *= XXH_PRIME64_3;
    Hash ^= Hash >> 32;
    return Hash;
}

uint64_t VgmXxh64Compute(const void *Data, size_t Len, uint64_t Seed)
{
    const uint8_t *p = (const uint8_t *)Data;
    const uint8_t *end = p + Len;
    uint64_t h64;

    if (Len >= 32) {
        const uint8_t *limit = end - 32;
        uint64_t v1 = Seed + XXH_PRIME64_1 + XXH_PRIME64_2;
        uint64_t v2 = Seed + XXH_PRIME64_2;
        uint64_t v3 = Seed + 0;
        uint64_t v4 = Seed - XXH_PRIME64_1;

        do {
            uint64_t k1, k2, k3, k4;
            memcpy(&k1, p +  0, 8);
            memcpy(&k2, p +  8, 8);
            memcpy(&k3, p + 16, 8);
            memcpy(&k4, p + 24, 8);
            v1 = VgmXxh64Round(v1, k1);
            v2 = VgmXxh64Round(v2, k2);
            v3 = VgmXxh64Round(v3, k3);
            v4 = VgmXxh64Round(v4, k4);
            p += 32;
        } while (p <= limit);

        h64 = _rotl64(v1, 1) + _rotl64(v2, 7) + _rotl64(v3, 12) + _rotl64(v4, 18);
    } else {
        h64 = Seed + XXH_PRIME64_5;
    }

    h64 += (uint64_t)Len;

    while (p + 8 <= end) {
        uint64_t k1;
        memcpy(&k1, p, 8);
        k1 *= XXH_PRIME64_2;
        k1  = _rotl64(k1, 31);
        k1 *= XXH_PRIME64_1;
        h64 ^= k1;
        h64  = _rotl64(h64, 27) * XXH_PRIME64_1 + XXH_PRIME64_4;
        p += 8;
    }

    if (p + 4 <= end) {
        uint32_t k1;
        memcpy(&k1, p, 4);
        h64 ^= (uint64_t)k1 * XXH_PRIME64_1;
        h64  = _rotl64(h64, 23) * XXH_PRIME64_2 + XXH_PRIME64_3;
        p += 4;
    }

    while (p < end) {
        h64 ^= (*p) * XXH_PRIME64_5;
        h64  = _rotl64(h64, 11) * XXH_PRIME64_1;
        p++;
    }

    return VgmXxh64Avalanche(h64);
}

VGM_XXH128_HASH VgmXxh3_128Compute(const void *Data, size_t Len, uint64_t Seed)
{
    VGM_XXH128_HASH result;
    result.Low64  = VgmXxh64Compute(Data, Len, Seed);
    result.High64 = VgmXxh64Compute(Data, Len, Seed ^ XXH_PRIME64_1 ^ (uint64_t)Len);
    return result;
}

/* ============================================================================
 * 5. CUSTOM %wZ / %Z FORMATTER (FUN_1419c3d8d)
 * ============================================================================ */

int VgmFormatUnicodeString(wchar_t *OutBuf, size_t MaxChars, const wchar_t *Format, ...)
{
    if (OutBuf == NULL || MaxChars == 0 || Format == NULL) {
        return -1;
    }

    va_list args;
    va_start(args, Format);
    int written = _vsnwprintf_s(OutBuf, MaxChars, _TRUNCATE, Format, args);
    va_end(args);
    return written;
}
