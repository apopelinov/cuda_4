#include "cuda_runtime.h"
#include "device_launch_parameters.h"
#include "CUDA.h"
#include "Consts.h"

__host__ __device__ void GpuToCompressedSize(int w, int h, int & cw, int & ch)
{
	cw = w / BLOCK_SIZE * COMPRESSED_BLOCK_SIZE;
	ch = h / BLOCK_SIZE * COMPRESSED_BLOCK_SIZE;
}

__device__ void CosineTransform(byte_t src[BLOCK_SIZE][BLOCK_SIZE],
								byte_t dst[COMPRESSED_BLOCK_SIZE][COMPRESSED_BLOCK_SIZE])
{
	int u = threadIdx.x;
	int v = threadIdx.y;
	if (u >= COMPRESSED_BLOCK_SIZE || v >= COMPRESSED_BLOCK_SIZE) return;
	int uu = ZZ[u][v][0], vv = ZZ[u][v][1];
	float uv = 0;
	for (int li = 0; li < BLOCK_SIZE; ++li)
	for (int lj = 0; lj < BLOCK_SIZE; ++lj)
	{
		uv += CC[li][uu] * CC[lj][vv] * src[li][lj] / QY[uu][vv];
	}
	uv = fmaxf(-127.f, fminf(127.f, uv));
	dst[u][v] = (byte_t)((char)uv);
}

__device__ void CosineTransformInv(byte_t dst[BLOCK_SIZE][BLOCK_SIZE],
								   byte_t src[COMPRESSED_BLOCK_SIZE][COMPRESSED_BLOCK_SIZE])
{
	int li = threadIdx.x;
	int lj = threadIdx.y;
	float uv = 0;
	for (int u = 0; u < COMPRESSED_BLOCK_SIZE; ++u)
	for (int v = 0; v < COMPRESSED_BLOCK_SIZE; ++v)
	{
		int uu = ZZ[u][v][0], vv = ZZ[u][v][1];
		float c = (char)src[u][v] * QY[uu][vv];

		uv += CC[li][uu] * CC[lj][vv] * c;
	}
	dst[li][lj] = (byte_t)(fmaxf(0.f, fminf(255.f, uv)));
}

__global__ void DoCompress(const byte_t * src, byte_t * dst, int w, int h)
{
	int cw, ch; GpuToCompressedSize(w, h, cw, ch);

	int bi = blockIdx.x;
	int bj = blockIdx.y;

	int ti = threadIdx.x;
	int tj = threadIdx.y;

	int i = bi * BLOCK_SIZE + ti;
	int j = bj * BLOCK_SIZE + tj;

	int ci = bi * COMPRESSED_BLOCK_SIZE + ti;
	int cj = bj * COMPRESSED_BLOCK_SIZE + tj;

	__shared__ byte_t block[BLOCK_SIZE][BLOCK_SIZE];
	__shared__ byte_t buf[COMPRESSED_BLOCK_SIZE][COMPRESSED_BLOCK_SIZE];

	block[ti][tj] = src[i * w + j];

	__syncthreads();

	CosineTransform(block, buf);

	__syncthreads();

	if (ti < COMPRESSED_BLOCK_SIZE && tj < COMPRESSED_BLOCK_SIZE)
		dst[ci * cw + cj] = buf[ti][tj];
}

__global__ void DoDecompress(const byte_t * src, byte_t * dst, int w, int h)
{
	int cw, ch; GpuToCompressedSize(w, h, cw, ch);

	int bi = blockIdx.x;
	int bj = blockIdx.y;

	int ti = threadIdx.x;
	int tj = threadIdx.y;

	int i = bi * BLOCK_SIZE + ti;
	int j = bj * BLOCK_SIZE + tj;

	int ci = bi * COMPRESSED_BLOCK_SIZE + ti;
	int cj = bj * COMPRESSED_BLOCK_SIZE + tj;

	__shared__ byte_t block[BLOCK_SIZE][BLOCK_SIZE];
	__shared__ byte_t buf[COMPRESSED_BLOCK_SIZE][COMPRESSED_BLOCK_SIZE];

	if (ti < COMPRESSED_BLOCK_SIZE && tj < COMPRESSED_BLOCK_SIZE)
		buf[ti][tj] = src[ci * cw + cj];

	__syncthreads();

	CosineTransformInv(block, buf);

	__syncthreads();

	dst[i * w + j] = block[ti][tj];
}

void StartDeviceCompress(byte_t * src, byte_t * dst, int w, int h)
{
	dim3 threads(BLOCK_SIZE, BLOCK_SIZE);
	dim3 blocks(w / BLOCK_SIZE, h / BLOCK_SIZE);

	int cw, ch; GpuToCompressedSize(w, h, cw, ch);

	byte_t * csrc, * cdst;

	cudaMalloc(&csrc, w * h);
	cudaMalloc(&cdst, cw * ch);

	cudaMemcpy(csrc, src, sizeof(byte_t) * w * h, cudaMemcpyHostToDevice);

	DoCompress<<<blocks, threads>>>(csrc, cdst, w, h);

	cudaDeviceSynchronize();

	cudaMemcpy(dst, cdst, sizeof(byte_t) * cw * ch, cudaMemcpyDeviceToHost);

	cudaFree(csrc);
	cudaFree(cdst);
}

void StartDeviceDecompress(byte_t * dst, byte_t * src, int w, int h)
{
	dim3 threads(BLOCK_SIZE, BLOCK_SIZE);
	dim3 blocks(w / BLOCK_SIZE, h / BLOCK_SIZE);

	int cw, ch; GpuToCompressedSize(w, h, cw, ch);

	byte_t * csrc, *cdst;

	cudaMalloc(&csrc, cw * ch);
	cudaMalloc(&cdst, w * h);

	cudaMemcpy(csrc, src, sizeof(byte_t) * cw * ch, cudaMemcpyHostToDevice);

	DoDecompress<<<blocks, threads>>>(csrc, cdst, w, h);

	cudaDeviceSynchronize();

	cudaMemcpy(dst, cdst, sizeof(byte_t) * w * h, cudaMemcpyDeviceToHost);

	cudaFree(csrc);
	cudaFree(cdst);
}
