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
#define MD5_GET(n) (ctx->block[(n)])
#define MD5_SET(n) (ctx->block[(n)] = \
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
	uint8_t input[64];
	uint32_t block[16];
} md5_ctx;

static size_t md5_process_block(md5_ctx *ctx, const uint8_t *ptr, size_t size)
{
	uint32_t a, b, c, d, aa, bb, cc, dd;
	size_t rem = size;

	if (rem < 64) {
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

		MD5_STEP(MD5_F, a, b, c, d, MD5_SET(0), 0xd76aa478, 7);
		MD5_STEP(MD5_F, d, a, b, c, MD5_SET(1), 0xe8c7b756, 12);
		MD5_STEP(MD5_F, c, d, a, b, MD5_SET(2), 0x242070db, 17);
		MD5_STEP(MD5_F, b, c, d, a, MD5_SET(3), 0xc1bdceee, 22);
		MD5_STEP(MD5_F, a, b, c, d, MD5_SET(4), 0xf57c0faf, 7);
		MD5_STEP(MD5_F, d, a, b, c, MD5_SET(5), 0x4787c62a, 12);
		MD5_STEP(MD5_F, c, d, a, b, MD5_SET(6), 0xa8304613, 17);
		MD5_STEP(MD5_F, b, c, d, a, MD5_SET(7), 0xfd469501, 22);
		MD5_STEP(MD5_F, a, b, c, d, MD5_SET(8), 0x698098d8, 7);
		MD5_STEP(MD5_F, d, a, b, c, MD5_SET(9), 0x8b44f7af, 12);
		MD5_STEP(MD5_F, c, d, a, b, MD5_SET(10), 0xffff5bb1, 17);
		MD5_STEP(MD5_F, b, c, d, a, MD5_SET(11), 0x895cd7be, 22);
		MD5_STEP(MD5_F, a, b, c, d, MD5_SET(12), 0x6b901122, 7);
		MD5_STEP(MD5_F, d, a, b, c, MD5_SET(13), 0xfd987193, 12);
		MD5_STEP(MD5_F, c, d, a, b, MD5_SET(14), 0xa679438e, 17);
		MD5_STEP(MD5_F, b, c, d, a, MD5_SET(15), 0x49b40821, 22);

		MD5_STEP(MD5_G, a, b, c, d, MD5_GET(1), 0xf61e2562, 5);
		MD5_STEP(MD5_G, d, a, b, c, MD5_GET(6), 0xc040b340, 9);
		MD5_STEP(MD5_G, c, d, a, b, MD5_GET(11), 0x265e5a51, 14);
		MD5_STEP(MD5_G, b, c, d, a, MD5_GET(0), 0xe9b6c7aa, 20);
		MD5_STEP(MD5_G, a, b, c, d, MD5_GET(5), 0xd62f105d, 5);
		MD5_STEP(MD5_G, d, a, b, c, MD5_GET(10), 0x02441453, 9);
		MD5_STEP(MD5_G, c, d, a, b, MD5_GET(15), 0xd8a1e681, 14);
		MD5_STEP(MD5_G, b, c, d, a, MD5_GET(4), 0xe7d3fbc8, 20);
		MD5_STEP(MD5_G, a, b, c, d, MD5_GET(9), 0x21e1cde6, 5);
		MD5_STEP(MD5_G, d, a, b, c, MD5_GET(14), 0xc33707d6, 9);
		MD5_STEP(MD5_G, c, d, a, b, MD5_GET(3), 0xf4d50d87, 14);
		MD5_STEP(MD5_G, b, c, d, a, MD5_GET(8), 0x455a14ed, 20);
		MD5_STEP(MD5_G, a, b, c, d, MD5_GET(13), 0xa9e3e905, 5);
		MD5_STEP(MD5_G, d, a, b, c, MD5_GET(2), 0xfcefa3f8, 9);
		MD5_STEP(MD5_G, c, d, a, b, MD5_GET(7), 0x676f02d9, 14);
		MD5_STEP(MD5_G, b, c, d, a, MD5_GET(12), 0x8d2a4c8a, 20);

		MD5_STEP(MD5_H, a, b, c, d, MD5_GET(5), 0xfffa3942, 4);
		MD5_STEP(MD5_H, d, a, b, c, MD5_GET(8), 0x8771f681, 11);
		MD5_STEP(MD5_H, c, d, a, b, MD5_GET(11), 0x6d9d6122, 16);
		MD5_STEP(MD5_H, b, c, d, a, MD5_GET(14), 0xfde5380c, 23);
		MD5_STEP(MD5_H, a, b, c, d, MD5_GET(1), 0xa4beea44, 4);
		MD5_STEP(MD5_H, d, a, b, c, MD5_GET(4), 0x4bdecfa9, 11);
		MD5_STEP(MD5_H, c, d, a, b, MD5_GET(7), 0xf6bb4b60, 16);
		MD5_STEP(MD5_H, b, c, d, a, MD5_GET(10), 0xbebfbc70, 23);
		MD5_STEP(MD5_H, a, b, c, d, MD5_GET(13), 0x289b7ec6, 4);
		MD5_STEP(MD5_H, d, a, b, c, MD5_GET(0), 0xeaa127fa, 11);
		MD5_STEP(MD5_H, c, d, a, b, MD5_GET(3), 0xd4ef3085, 16);
		MD5_STEP(MD5_H, b, c, d, a, MD5_GET(6), 0x04881d05, 23);
		MD5_STEP(MD5_H, a, b, c, d, MD5_GET(9), 0xd9d4d039, 4);
		MD5_STEP(MD5_H, d, a, b, c, MD5_GET(12), 0xe6db99e5, 11);
		MD5_STEP(MD5_H, c, d, a, b, MD5_GET(15), 0x1fa27cf8, 16);
		MD5_STEP(MD5_H, b, c, d, a, MD5_GET(2), 0xc4ac5665, 23);

		MD5_STEP(MD5_I, a, b, c, d, MD5_GET(0), 0xf4292244, 6);
		MD5_STEP(MD5_I, d, a, b, c, MD5_GET(7), 0x432aff97, 10);
		MD5_STEP(MD5_I, c, d, a, b, MD5_GET(14), 0xab9423a7, 15);
		MD5_STEP(MD5_I, b, c, d, a, MD5_GET(5), 0xfc93a039, 21);
		MD5_STEP(MD5_I, a, b, c, d, MD5_GET(12), 0x655b59c3, 6);
		MD5_STEP(MD5_I, d, a, b, c, MD5_GET(3), 0x8f0ccc92, 10);
		MD5_STEP(MD5_I, c, d, a, b, MD5_GET(10), 0xffeff47d, 15);
		MD5_STEP(MD5_I, b, c, d, a, MD5_GET(1), 0x85845dd1, 21);
		MD5_STEP(MD5_I, a, b, c, d, MD5_GET(8), 0x6fa87e4f, 6);
		MD5_STEP(MD5_I, d, a, b, c, MD5_GET(15), 0xfe2ce6e0, 10);
		MD5_STEP(MD5_I, c, d, a, b, MD5_GET(6), 0xa3014314, 15);
		MD5_STEP(MD5_I, b, c, d, a, MD5_GET(13), 0x4e0811a1, 21);
		MD5_STEP(MD5_I, a, b, c, d, MD5_GET(4), 0xf7537e82, 6);
		MD5_STEP(MD5_I, d, a, b, c, MD5_GET(11), 0xbd3af235, 10);
		MD5_STEP(MD5_I, c, d, a, b, MD5_GET(2), 0x2ad7d2bb, 15);
		MD5_STEP(MD5_I, b, c, d, a, MD5_GET(9), 0xeb86d391, 21);

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
	ctx->b = 0xefcdab89;
	ctx->c = 0x98badcfe;
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
		if (copy_bytes > buffer_size) {
			copy_bytes = buffer_size;
		}
		memcpy(&ctx->input[bytes], buffer, copy_bytes);
		bytes += copy_bytes;
		buffer += copy_bytes;
		buffer_size -= copy_bytes;
		if (bytes == 64) {
			md5_process_block(ctx, ctx->input, 64);
		}
	}

	bytes = md5_process_block(ctx, buffer, buffer_size);
	buffer += bytes;
	buffer_size -= bytes;
	if (buffer_size > 0) {
		memcpy(ctx->input, buffer, buffer_size);
	}
}

static void md5_digest(md5_ctx *ctx, uint8_t *digest)
{
	size_t free;
	uint64_t bits_count;
	size_t used = ctx->count & 63;

	ctx->input[used++] = 0x80;
	free = 64 - used;

	if (free < 8)
	{
		memset(&ctx->input[used], 0, free);
		md5_process_block(ctx, ctx->input, 64);
		used = 0;
		free = 64;
	}

	memset(&ctx->input[used], 0, free - 8);

	bits_count = ctx->count << 3;
	ctx->input[56] = (uint8_t)(bits_count >> 0);
	ctx->input[57] = (uint8_t)(bits_count >> 8);
	ctx->input[58] = (uint8_t)(bits_count >> 16);
	ctx->input[59] = (uint8_t)(bits_count >> 24);
	ctx->input[60] = (uint8_t)(bits_count >> 32);
	ctx->input[61] = (uint8_t)(bits_count >> 40);
	ctx->input[62] = (uint8_t)(bits_count >> 48);
	ctx->input[63] = (uint8_t)(bits_count >> 56);

	md5_process_block(ctx, ctx->input, 64);

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
