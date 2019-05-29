#include "pch.h"

#include "compress.h"

void GpuCompress(byte_t * grayBmp, byte_t * coefs, int w, int h)
{
	StartDeviceCompress(grayBmp, coefs, w, h);
}

void GpuDecompress(byte_t * grayBmp, byte_t * coefs, int w, int h)
{
	StartDeviceDecompress(grayBmp, coefs, w, h);
}
