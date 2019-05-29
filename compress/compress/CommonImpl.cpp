#include "pch.h"

#include <chrono>

#include "compress.h"

static void Test(CString imgPath, int scale, int time[4])
{
	byte_t * src, *img1, *img2, *coefs1, *coefs2;
	int w, h;

	if (scale != 0)
	{
		printf("\n--------\nИзображение %d x %d\n\n", 1 << scale, 1 << scale);
	}

	if (!ReadImg(imgPath, scale, src, w, h))
	{
		printf("Не удается загрузить файл [%s]", imgPath.GetBuffer());
		return;
	}

	CreateImg(img1, w, h);
	CreateImg(img2, w, h);
	CreateCoefs(coefs1, w, h);
	CreateCoefs(coefs2, w, h);

	for (int i = 0; i < WARMUP; ++i) {
		if (scale > UPSCALE_PRINT) printf("сжатие cpu - прогрев: %d\n", i + 1);
		CpuCompress(src, coefs1, w, h);
	}
	auto begin1 = std::chrono::steady_clock::now();
	for (int i = 0; i < TESTS; ++i) {
		if (scale > UPSCALE_PRINT) printf("сжатие cpu: %d\n", i + 1);
		CpuCompress(src, coefs1, w, h);
	}
	auto end1 = std::chrono::steady_clock::now();

	for (int i = 0; i < WARMUP; ++i) {
		if (scale > UPSCALE_PRINT) printf("разжатие cpu - прогрев: %d\n", i + 1);
		CpuDecompress(img1, coefs1, w, h);
	}
	auto begin2 = std::chrono::steady_clock::now();
	for (int i = 0; i < TESTS; ++i) {
		if (scale > UPSCALE_PRINT) printf("разжатие cpu: %d\n", i + 1);
		CpuDecompress(img1, coefs1, w, h);
	}
	auto end2 = std::chrono::steady_clock::now();

	for (int i = 0; i < WARMUP; ++i) {
		if (scale > UPSCALE_PRINT) printf("сжатие gpu - прогрев: %d\n", i + 1);
		GpuCompress(src, coefs2, w, h);
	}
	auto begin3 = std::chrono::steady_clock::now();
	for (int i = 0; i < TESTS; ++i) {
		if (scale > UPSCALE_PRINT) printf("сжатие gpu: %d\n", i + 1);
		GpuCompress(src, coefs2, w, h);
	}
	auto end3 = std::chrono::steady_clock::now();

	for (int i = 0; i < WARMUP; ++i) {
		if (scale > UPSCALE_PRINT) printf("разжатие gpu - прогрев: %d\n", i + 1);
		GpuDecompress(img2, coefs2, w, h);
	}
	auto begin4 = std::chrono::steady_clock::now();
	for (int i = 0; i < TESTS; ++i) {
		if (scale > UPSCALE_PRINT) printf("разжатие gpu: %d\n", i + 1);
		GpuDecompress(img2, coefs2, w, h);
	}
	auto end4 = std::chrono::steady_clock::now();

	auto ts1 = std::chrono::duration_cast<std::chrono::microseconds>(end1 - begin1).count() / TESTS;
	auto ts2 = std::chrono::duration_cast<std::chrono::microseconds>(end2 - begin2).count() / TESTS;
	auto ts3 = std::chrono::duration_cast<std::chrono::microseconds>(end3 - begin3).count() / TESTS;
	auto ts4 = std::chrono::duration_cast<std::chrono::microseconds>(end4 - begin4).count() / TESTS;

	time[0] = (int)ts1;
	time[1] = (int)ts2;
	time[2] = (int)ts3;
	time[3] = (int)ts4;

	printf("Время работы cpu (мкс): %6d / %6d\n", (int)ts1, (int)ts2);
	printf("Время работы gpu (мкс): %6d / %6d\n", (int)ts3, (int)ts4);

	if (scale == 0)
	{
		CreateDirectory(L"output", NULL);

		WriteImg("output\\src.png", src, w, h);
		WriteCoefs("output\\cpuCoefs.dat", coefs1, w, h);
		WriteCoefs("output\\gpuCoefs.dat", coefs2, w, h);
		WriteImg("output\\cpuImg.png", img1, w, h);
		WriteImg("output\\gpuImg.png", img2, w, h);
	}

	FreeImg(src, w, h);
	FreeImg(img1, w, h);
	FreeImg(img2, w, h);
	FreeCoefs(coefs1, w, h);
	FreeCoefs(coefs2, w, h);
}

void Test(CString imgPath)
{
	int time[4];
	Test(imgPath, 0, time);
}

void Compute(CString imgPath)
{
	int time[UPSCALE_MAX - UPSCALE_MIN + 1][4];
	for (int i = UPSCALE_MIN; i <= UPSCALE_MAX; ++i)
	{
		Test(imgPath, i, time[i - UPSCALE_MIN]);
	}
	printf("\n=========\nРезультаты:\n\n");
	printf("размер | сжатие cpu | gpu | разжатие cpu | gpu\n");
	for (int i = UPSCALE_MIN; i <= UPSCALE_MAX; ++i)
	{
		printf("%d | %d | %d | %d | %d\n", 1 << i,
			time[i - UPSCALE_MIN][0], time[i - UPSCALE_MIN][2],
			time[i - UPSCALE_MIN][1], time[i - UPSCALE_MIN][3]);
	}
}

union color_t
{
	struct
	{
		byte_t r;
		byte_t g;
		byte_t b;
		byte_t a;
	};
	COLORREF c;
};

bool ReadImg(CString imgPath, int scale, byte_t *& grayBmp, int & w, int & h)
{
	CImage img;
	if (FAILED(img.Load(imgPath)))
	{
		return false;
	}

	int ow = img.GetWidth();
	int oh = img.GetHeight();

	w = (ow * BLOCK_SIZE + (BLOCK_SIZE - 1)) / BLOCK_SIZE;
	h = (oh * BLOCK_SIZE + (BLOCK_SIZE - 1)) / BLOCK_SIZE;
	CBitmap bmp; bmp.Attach(img.Detach());

	if (scale == 0)
	{
		CreateImg(grayBmp, w, h);

		BITMAP handle; bmp.GetBitmap(&handle);
		for (size_t i = 0; i < h; ++i)
		for (size_t j = 0; j < w; ++j)
		{
			color_t c;
			c.r = ((byte_t *)handle.bmBits)[((w - 1 - i) * w + j) * 3 + 0];
			c.g = ((byte_t *)handle.bmBits)[((w - 1 - i) * w + j) * 3 + 1];
			c.b = ((byte_t *)handle.bmBits)[((w - 1 - i) * w + j) * 3 + 2];
			double y = c.r * 0.299 + c.g * 0.587 + c.b * 0.114;
			grayBmp[i * w + j] = (byte_t)(max(0, min(255, y)));
		}
	}
	else
	{
		int nw = 1 << scale;

		w = nw; h = nw;

		CreateImg(grayBmp, nw, nw);

		byte_t * bits = new byte_t[4 * w * h];
		CBitmap scaled;
		scaled.CreateBitmap(nw, nw, 1, 32, bits);

		CDC dc; dc.CreateCompatibleDC(nullptr);
		CDC dc2; dc2.CreateCompatibleDC(nullptr);
		dc.SelectObject(&scaled);
		dc2.SelectObject(&bmp);
		dc.StretchBlt(0, 0, nw, nw, &dc2, 0, 0, ow, oh, SRCCOPY);
		for (size_t i = 0; i < h; ++i)
		for (size_t j = 0; j < w; ++j)
		{
			color_t c;
			c.r = bits[((w - 1 - i) * w + j) * 4 + 0];
			c.g = bits[((w - 1 - i) * w + j) * 4 + 1];
			c.b = bits[((w - 1 - i) * w + j) * 4 + 2];
			double y = c.r * 0.299 + c.g * 0.587 + c.b * 0.114;
			grayBmp[i * nw + j] = (byte_t)(max(0, min(255, y)));
		}
		delete[] bits;
	}

	return true;
}

bool WriteImg(CString imgPath, byte_t * grayBmp, int w, int h)
{
	CDC dc; dc.CreateCompatibleDC(nullptr);
	CBitmap bmp;
	bmp.CreateBitmap(w, h, 1, 32, NULL);
	dc.SelectObject(&bmp);
	for (size_t i = 0; i < h; ++i)
	for (size_t j = 0; j < w; ++j)
	{
		dc.SetPixel(j, i, RGB(grayBmp[i * w + j], grayBmp[i * w + j], grayBmp[i * w + j]));
	}
	CImage img; img.Attach((HBITMAP)bmp.Detach());
	return SUCCEEDED(img.Save(imgPath));
}

bool WriteCoefs(CString coefsPath, byte_t * coefs, int w, int h)
{
	ToCompressedSize(w, h, w, h);

	CFile file;
	if (FAILED(file.Open(coefsPath, CFile::modeCreate | CFile::modeWrite))) return false;

	file.Write(coefs, sizeof(byte_t) * w * h);

	return true;
}

void ToCompressedSize(int w, int h, int & cw, int & ch)
{
	cw = w / BLOCK_SIZE * COMPRESSED_BLOCK_SIZE;
	ch = h / BLOCK_SIZE * COMPRESSED_BLOCK_SIZE;
}

void CreateImg(byte_t *& grayImg, int w, int h)
{
	grayImg = new byte_t[w * h]();
}
void CreateCoefs(byte_t *& coefs, int w, int h)
{
	ToCompressedSize(w, h, w, h);
	coefs = new byte_t[w * h]();
}
void FreeImg(byte_t * grayImg, int w, int h)
{
	delete[] grayImg;
}
void FreeCoefs(byte_t * coefs, int w, int h)
{
	delete[] coefs;
}
