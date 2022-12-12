/*!
 * Copyright (c) 2022, Dominik Kilian <kontakt@dominik.cc>
 * All rights reserved.
 *
 * This software is distributed under the BSD 3-Clause License. See the
 * LICENSE.txt file for details.
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _MD5_H_
#define _MD5_H_

#include <memory.h>
#include <stdint.h>
#include <stdbool.h>

#define MD5_ROTATE_LEFT(x, s) (((x) << (s)) | (x >> (32 - (s))))

#define MD5_STEP(f, a, b, c, d, x, t, s) ( \
    (a) += f(b, c, d) + (x) + (t),         \
    (a) = MD5_ROTATE_LEFT(a, s),           \
    (a) += (b))

#define MD5_F(x, y, z) (z ^ (x & (y ^ z)))
#define MD5_G(x, y, z) (y ^ (z & (x ^ y)))
#define MD5_H(x, y, z) (x ^ y ^ z)
#define MD5_I(x, y, z) (y ^ (x | ~z))
#define MD5_TEMP_GET(n) (ctx->temp[(n)])
#define MD5_TEMP_SET(n) (ctx->temp[(n)] = \
                             ((uint32_t)ptr[(n)*4 + 0] << 0) | \
                             ((uint32_t)ptr[(n)*4 + 1] << 8) | \
                             ((uint32_t)ptr[(n)*4 + 2] << 16) | \
                             ((uint32_t)ptr[(n)*4 + 3] << 24))

typedef struct
{
    uint32_t a;
    uint32_t b;
    uint32_t c;
    uint32_t d;
    uint64_t count;
    uint8_t buffer[64];
    uint32_t temp[16];
} md5_ctx;

static size_t md5_process_block(md5_ctx *ctx, const uint8_t *ptr, size_t size)
{
    uint32_t a, b, c, d, aa, bb, cc, dd;
    size_t rem = size;

    if (rem < 64)
    {
        return 0;
    }

    a = ctx->a;
    b = ctx->b;
    c = ctx->c;
    d = ctx->d;

    while (rem >= 64)
    {
        aa = a;
        bb = b;
        cc = c;
        dd = d;

        MD5_STEP(MD5_F, a, b, c, d, MD5_TEMP_SET(0), 0xD76AA478, 7);
        MD5_STEP(MD5_F, d, a, b, c, MD5_TEMP_SET(1), 0xE8C7B756, 12);
        MD5_STEP(MD5_F, c, d, a, b, MD5_TEMP_SET(2), 0x242070DB, 17);
        MD5_STEP(MD5_F, b, c, d, a, MD5_TEMP_SET(3), 0xC1BDCEEE, 22);
        MD5_STEP(MD5_F, a, b, c, d, MD5_TEMP_SET(4), 0xF57C0FAF, 7);
        MD5_STEP(MD5_F, d, a, b, c, MD5_TEMP_SET(5), 0x4787C62A, 12);
        MD5_STEP(MD5_F, c, d, a, b, MD5_TEMP_SET(6), 0xA8304613, 17);
        MD5_STEP(MD5_F, b, c, d, a, MD5_TEMP_SET(7), 0xFD469501, 22);
        MD5_STEP(MD5_F, a, b, c, d, MD5_TEMP_SET(8), 0x698098D8, 7);
        MD5_STEP(MD5_F, d, a, b, c, MD5_TEMP_SET(9), 0x8B44F7AF, 12);
        MD5_STEP(MD5_F, c, d, a, b, MD5_TEMP_SET(10), 0xFFFF5BB1, 17);
        MD5_STEP(MD5_F, b, c, d, a, MD5_TEMP_SET(11), 0x895CD7BE, 22);
        MD5_STEP(MD5_F, a, b, c, d, MD5_TEMP_SET(12), 0x6B901122, 7);
        MD5_STEP(MD5_F, d, a, b, c, MD5_TEMP_SET(13), 0xFD987193, 12);
        MD5_STEP(MD5_F, c, d, a, b, MD5_TEMP_SET(14), 0xA679438E, 17);
        MD5_STEP(MD5_F, b, c, d, a, MD5_TEMP_SET(15), 0x49B40821, 22);

        MD5_STEP(MD5_G, a, b, c, d, MD5_TEMP_GET(1), 0xF61E2562, 5);
        MD5_STEP(MD5_G, d, a, b, c, MD5_TEMP_GET(6), 0xC040B340, 9);
        MD5_STEP(MD5_G, c, d, a, b, MD5_TEMP_GET(11), 0x265E5A51, 14);
        MD5_STEP(MD5_G, b, c, d, a, MD5_TEMP_GET(0), 0xE9B6C7AA, 20);
        MD5_STEP(MD5_G, a, b, c, d, MD5_TEMP_GET(5), 0xD62F105D, 5);
        MD5_STEP(MD5_G, d, a, b, c, MD5_TEMP_GET(10), 0x02441453, 9);
        MD5_STEP(MD5_G, c, d, a, b, MD5_TEMP_GET(15), 0xD8A1E681, 14);
        MD5_STEP(MD5_G, b, c, d, a, MD5_TEMP_GET(4), 0xE7D3FBC8, 20);
        MD5_STEP(MD5_G, a, b, c, d, MD5_TEMP_GET(9), 0x21E1CDE6, 5);
        MD5_STEP(MD5_G, d, a, b, c, MD5_TEMP_GET(14), 0xC33707D6, 9);
        MD5_STEP(MD5_G, c, d, a, b, MD5_TEMP_GET(3), 0xF4D50D87, 14);
        MD5_STEP(MD5_G, b, c, d, a, MD5_TEMP_GET(8), 0x455A14ED, 20);
        MD5_STEP(MD5_G, a, b, c, d, MD5_TEMP_GET(13), 0xA9E3E905, 5);
        MD5_STEP(MD5_G, d, a, b, c, MD5_TEMP_GET(2), 0xFCEFA3F8, 9);
        MD5_STEP(MD5_G, c, d, a, b, MD5_TEMP_GET(7), 0x676F02D9, 14);
        MD5_STEP(MD5_G, b, c, d, a, MD5_TEMP_GET(12), 0x8D2A4C8A, 20);

        MD5_STEP(MD5_H, a, b, c, d, MD5_TEMP_GET(5), 0xFFFA3942, 4);
        MD5_STEP(MD5_H, d, a, b, c, MD5_TEMP_GET(8), 0x8771F681, 11);
        MD5_STEP(MD5_H, c, d, a, b, MD5_TEMP_GET(11), 0x6D9D6122, 16);
        MD5_STEP(MD5_H, b, c, d, a, MD5_TEMP_GET(14), 0xFDE5380C, 23);
        MD5_STEP(MD5_H, a, b, c, d, MD5_TEMP_GET(1), 0xA4BEEA44, 4);
        MD5_STEP(MD5_H, d, a, b, c, MD5_TEMP_GET(4), 0x4BDECFA9, 11);
        MD5_STEP(MD5_H, c, d, a, b, MD5_TEMP_GET(7), 0xF6BB4B60, 16);
        MD5_STEP(MD5_H, b, c, d, a, MD5_TEMP_GET(10), 0xBEBFBC70, 23);
        MD5_STEP(MD5_H, a, b, c, d, MD5_TEMP_GET(13), 0x289B7EC6, 4);
        MD5_STEP(MD5_H, d, a, b, c, MD5_TEMP_GET(0), 0xEAA127FA, 11);
        MD5_STEP(MD5_H, c, d, a, b, MD5_TEMP_GET(3), 0xD4EF3085, 16);
        MD5_STEP(MD5_H, b, c, d, a, MD5_TEMP_GET(6), 0x04881D05, 23);
        MD5_STEP(MD5_H, a, b, c, d, MD5_TEMP_GET(9), 0xD9D4D039, 4);
        MD5_STEP(MD5_H, d, a, b, c, MD5_TEMP_GET(12), 0xE6DB99E5, 11);
        MD5_STEP(MD5_H, c, d, a, b, MD5_TEMP_GET(15), 0x1FA27CF8, 16);
        MD5_STEP(MD5_H, b, c, d, a, MD5_TEMP_GET(2), 0xC4AC5665, 23);

        MD5_STEP(MD5_I, a, b, c, d, MD5_TEMP_GET(0), 0xF4292244, 6);
        MD5_STEP(MD5_I, d, a, b, c, MD5_TEMP_GET(7), 0x432AFF97, 10);
        MD5_STEP(MD5_I, c, d, a, b, MD5_TEMP_GET(14), 0xAB9423A7, 15);
        MD5_STEP(MD5_I, b, c, d, a, MD5_TEMP_GET(5), 0xFC93A039, 21);
        MD5_STEP(MD5_I, a, b, c, d, MD5_TEMP_GET(12), 0x655B59C3, 6);
        MD5_STEP(MD5_I, d, a, b, c, MD5_TEMP_GET(3), 0x8F0CCC92, 10);
        MD5_STEP(MD5_I, c, d, a, b, MD5_TEMP_GET(10), 0xFFEFF47D, 15);
        MD5_STEP(MD5_I, b, c, d, a, MD5_TEMP_GET(1), 0x85845DD1, 21);
        MD5_STEP(MD5_I, a, b, c, d, MD5_TEMP_GET(8), 0x6FA87E4F, 6);
        MD5_STEP(MD5_I, d, a, b, c, MD5_TEMP_GET(15), 0xFE2CE6E0, 10);
        MD5_STEP(MD5_I, c, d, a, b, MD5_TEMP_GET(6), 0xA3014314, 15);
        MD5_STEP(MD5_I, b, c, d, a, MD5_TEMP_GET(13), 0x4E0811A1, 21);
        MD5_STEP(MD5_I, a, b, c, d, MD5_TEMP_GET(4), 0xF7537E82, 6);
        MD5_STEP(MD5_I, d, a, b, c, MD5_TEMP_GET(11), 0xBD3AF235, 10);
        MD5_STEP(MD5_I, c, d, a, b, MD5_TEMP_GET(2), 0x2AD7D2BB, 15);
        MD5_STEP(MD5_I, b, c, d, a, MD5_TEMP_GET(9), 0xEB86D391, 21);

        a += aa;
        b += bb;
        c += cc;
        d += dd;

        ptr += 64;
        rem -= 64;
    };

    ctx->a = a;
    ctx->b = b;
    ctx->c = c;
    ctx->d = d;

    return size - rem;
}

static void md5_init(md5_ctx *ctx)
{
    ctx->a = 0x67452301;
    ctx->b = 0xEFCDAB89;
    ctx->c = 0x98BADCFE;
    ctx->d = 0x10325476;
    ctx->count = 0;
}

static void md5_update(md5_ctx *ctx, const uint8_t *buffer, size_t buffer_size)
{
    size_t bytes;

    bytes = ctx->count & 63;
    ctx->count += buffer_size;
    if (bytes > 0)
    {
        size_t copy_bytes = 64 - bytes;
        if (copy_bytes > buffer_size)
        {
            copy_bytes = buffer_size;
        }
        memcpy(&ctx->buffer[bytes], buffer, copy_bytes);
        bytes += copy_bytes;
        buffer += copy_bytes;
        buffer_size -= copy_bytes;
        if (bytes == 64)
        {
            md5_process_block(ctx, ctx->buffer, 64);
        }
    }

    bytes = md5_process_block(ctx, buffer, buffer_size);
    buffer += bytes;
    buffer_size -= bytes;
    if (buffer_size > 0)
    {
        memcpy(ctx->buffer, buffer, buffer_size);
    }
}

static void md5_digest(md5_ctx *ctx, uint8_t *digest)
{
    size_t free;
    uint64_t bits_count;
    size_t used = ctx->count & 63;

    ctx->buffer[used++] = 0x80;
    free = 64 - used;

    if (free < 8)
    {
        memset(&ctx->buffer[used], 0, free);
        md5_process_block(ctx, ctx->buffer, 64);
        used = 0;
        free = 64;
    }

    memset(&ctx->buffer[used], 0, free - 8);

    bits_count = ctx->count << 3;
    ctx->buffer[56] = (uint8_t)(bits_count >> 0);
    ctx->buffer[57] = (uint8_t)(bits_count >> 8);
    ctx->buffer[58] = (uint8_t)(bits_count >> 16);
    ctx->buffer[59] = (uint8_t)(bits_count >> 24);
    ctx->buffer[60] = (uint8_t)(bits_count >> 32);
    ctx->buffer[61] = (uint8_t)(bits_count >> 40);
    ctx->buffer[62] = (uint8_t)(bits_count >> 48);
    ctx->buffer[63] = (uint8_t)(bits_count >> 56);

    md5_process_block(ctx, ctx->buffer, 64);

    digest[0] = (uint8_t)(ctx->a);
    digest[1] = (uint8_t)(ctx->a >> 8);
    digest[2] = (uint8_t)(ctx->a >> 16);
    digest[3] = (uint8_t)(ctx->a >> 24);
    digest[4] = (uint8_t)(ctx->b);
    digest[5] = (uint8_t)(ctx->b >> 8);
    digest[6] = (uint8_t)(ctx->b >> 16);
    digest[7] = (uint8_t)(ctx->b >> 24);
    digest[8] = (uint8_t)(ctx->c);
    digest[9] = (uint8_t)(ctx->c >> 8);
    digest[10] = (uint8_t)(ctx->c >> 16);
    digest[11] = (uint8_t)(ctx->c >> 24);
    digest[12] = (uint8_t)(ctx->d);
    digest[13] = (uint8_t)(ctx->d >> 8);
    digest[14] = (uint8_t)(ctx->d >> 16);
    digest[15] = (uint8_t)(ctx->d >> 24);
}

#endif /* _MD5_H_ */
