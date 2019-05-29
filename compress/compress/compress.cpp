// compress.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
//

#include "pch.h"
#include "compress.h"
#include "CUDA.h"

#include <chrono>
#include <iostream>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// Единственный объект приложения

CWinApp theApp;

using namespace std;

int wmain(int argc, const wchar_t ** argv)
{
	setlocale(LC_ALL, "Russian");

    int nRetCode = 0;

    HMODULE hModule = ::GetModuleHandle(nullptr);

    if (hModule != nullptr)
    {
        // инициализировать MFC, а также печать и сообщения об ошибках про сбое
        if (!AfxWinInit(hModule, nullptr, ::GetCommandLine(), 0))
        {
            // TODO: вставьте сюда код для приложения.
            printf("Критическая ошибка: сбой при инициализации MFC\n");
            nRetCode = 1;
        }
        else
        {
            // TODO: вставьте сюда код для приложения.
        }
    }
    else
    {
        // TODO: измените код ошибки в соответствии с потребностями
        printf("Критическая ошибка: сбой GetModuleHandle\n");
        nRetCode = 1;
    }

	bool test = true;
	CString file = "";

	if (argc < 3)
	{
		printf("Критическая ошибка: неправильная команда\n");
		nRetCode = 1;
	}
	else
	{
		if (CString(argv[1]) == "compute") test = false;
		file = argv[2];

		if (test) Test(file);
		else Compute(file);
	}

	getc(stdin);

    return nRetCode;
}
