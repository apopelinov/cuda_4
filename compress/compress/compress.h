#pragma once

#include "resource.h"
#include <afxwin.h>
#include <atlimage.h>

#include "CUDA.h"
#include "Consts.h"

void Test(CString imgPath);
void Compute(CString imgPath);

bool ReadImg(CString imgPath, int scale, byte_t *& grayBmp, int & w, int & h);
bool WriteImg(CString imgPath, byte_t * grayBmp, int w, int h);
bool WriteCoefs(CString coefsPath, byte_t * coefs, int w, int h);

void CpuCompress(byte_t * grayBmp, byte_t * coefs, int w, int h);
void CpuDecompress(byte_t * grayBmp, byte_t * coefs, int w, int h);
void GpuCompress(byte_t * grayBmp, byte_t * coefs, int w, int h);
void GpuDecompress(byte_t * grayBmp, byte_t * coefs, int w, int h);

void ToCompressedSize(int w, int h, int & cw, int & ch);
void CreateImg(byte_t *& grayImg, int w, int h);
void CreateCoefs(byte_t *& coefs, int w, int h);
void FreeImg(byte_t * grayImg, int w, int h);
void FreeCoefs(byte_t * coefs, int w, int h);
