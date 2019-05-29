#pragma once

typedef unsigned char byte_t;

void StartDeviceCompress(byte_t * src, byte_t * dst, int W, int H);
void StartDeviceDecompress(byte_t * src, byte_t * dst, int W, int H);
