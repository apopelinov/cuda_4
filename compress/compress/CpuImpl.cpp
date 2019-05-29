#include "pch.h"

#include <cmath>

#include "compress.h"

static void CosineTransform(byte_t * grayBmp, byte_t * coefs, int w, int h, int bi, int bj)
{
	int cw, ch; ToCompressedSize(w, h, cw, ch);
	for (int u = 0; u < COMPRESSED_BLOCK_SIZE; ++u)
	for (int v = 0; v < COMPRESSED_BLOCK_SIZE; ++v)
	{
		int uu = ZZ[u][v][0], vv = ZZ[u][v][1];
		float uv = 0;
		for (int li = 0; li < BLOCK_SIZE; ++li)
		for (int lj = 0; lj < BLOCK_SIZE; ++lj)
		{
			int i = bi * BLOCK_SIZE + li;
			int j = bj * BLOCK_SIZE + lj;
			uv += CC[li][uu] * CC[lj][vv] * grayBmp[i * w + j] / QY[uu][vv];
		}
		uv = max(-127.f, min(127.f, uv));
		int ci = bi * COMPRESSED_BLOCK_SIZE, cj = bj * COMPRESSED_BLOCK_SIZE;
		coefs[(ci + u) * cw + (cj + v)] = (byte_t)((char)uv);
	}
}

static void CosineTransformInv(byte_t * grayBmp, byte_t * coefs, int w, int h, int bi, int bj)
{
	int cw, ch; ToCompressedSize(w, h, cw, ch);
	for (int li = 0; li < BLOCK_SIZE; ++li)
	for (int lj = 0; lj < BLOCK_SIZE; ++lj)
	{
		float uv = 0;
		for (int u = 0; u < COMPRESSED_BLOCK_SIZE; ++u)
		for (int v = 0; v < COMPRESSED_BLOCK_SIZE; ++v)
		{
			int uu = ZZ[u][v][0], vv = ZZ[u][v][1];
			int ci = bi * COMPRESSED_BLOCK_SIZE, cj = bj * COMPRESSED_BLOCK_SIZE;
			float c = (char)coefs[(ci + u) * cw + (cj + v)] * QY[uu][vv];

			uv += CC[li][uu] * CC[lj][vv] * c;
		}

		int i = bi * BLOCK_SIZE + li;
		int j = bj * BLOCK_SIZE + lj;

		grayBmp[i * w + j] = (byte_t)(max(0.f, min(255.f, uv)));
	}
}

static void CompressBlock(byte_t * grayBmp, byte_t * coefs, int w, int h, int bi, int bj)
{
	CosineTransform(grayBmp, coefs, w, h, bi, bj);
}

static void DecompressBlock(byte_t * grayBmp, byte_t * coefs, int w, int h, int bi, int bj)
{
	CosineTransformInv(grayBmp, coefs, w, h, bi, bj);
}

void CpuCompress(byte_t * grayBmp, byte_t * coefs, int w, int h)
{
	for (int bi = 0; bi < h / BLOCK_SIZE; ++bi)
	for (int bj = 0; bj < w / BLOCK_SIZE; ++bj)
	{
		CompressBlock(grayBmp, coefs, w, h, bi, bj);
	}
}

void CpuDecompress(byte_t * grayBmp, byte_t * coefs, int w, int h)
{
	for (int bi = 0; bi < h / BLOCK_SIZE; ++bi)
	for (int bj = 0; bj < w / BLOCK_SIZE; ++bj)
	{
		DecompressBlock(grayBmp, coefs, w, h, bi, bj);
	}
}
